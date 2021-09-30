#include "application.h"

Application::Application(int argc, char **argv)
    : _audioOutput(AudioOutput<int16_t>::instance())
    , _track(0)
    , _poller(new Poller(*this)) {
    if (!LastFM::init()) {
        throw InitializationError();
    }

    _audioOutput->producer(&_drive);
    _audioOutput->init();

    _app = Gtk::Application::create(argc, argv, "com.nirjacobson.cdplayer");

    _builder = Gtk::Builder::create();
    _builder->add_from_resource("/ui/cdplayer.glade");

    _builder->get_widget("stack", _stack);
    _builder->get_widget("bluetoothBox", _bluetoothBox);
    _builder->get_widget("playerBox", _playerBox);
    _builder->get_widget("bluetoothButton", _bluetoothButton);
    _builder->get_widget("shutdownButton", _shutdownButton);
    _bluetoothButton->signal_clicked().connect(sigc::mem_fun(this, &Application::on_bluetooth_button));

    _discComponent = new DiscComponent(_builder);
    _discComponent->signal_eject_requested().connect(sigc::mem_fun(this, &Application::eject));
    _discComponent->signal_track_selected().connect(sigc::mem_fun(this, &Application::on_track_selected));

    _nowPlayingComponent = new NowPlayingComponent(_builder);
    _nowPlayingComponent->signal_button().connect(sigc::mem_fun(this, &Application::on_button));

    _bluetoothComponent = new BluetoothComponent(_builder);
    _bluetoothComponent->signal_connected().connect(sigc::mem_fun(this, &Application::on_bluetooth_connected));
    _bluetoothComponent->signal_done().connect(sigc::mem_fun(this, &Application::on_bluetooth_done));
    if (!_audioOutput->isDefault())
        _bluetoothComponent->on_device_initialization_complete(true);

    _shutdownButton->signal_clicked().connect(sigc::mem_fun(this, &Application::on_shutdown_button));

    _builder->get_widget("window", _window);
    _window->fullscreen();

    _dispatcher.connect(sigc::mem_fun(*this, &Application::on_notification_from_poller));

    _systemdProxy = Gio::DBus::Proxy::create_for_bus_sync(
                        Gio::DBus::BusType::BUS_TYPE_SYSTEM,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager");
}

Application::~Application() {
    delete _playerBox;
    delete _bluetoothBox;
    delete _shutdownButton;
    delete _bluetoothButton;
    delete _stack;
    delete _window;
    delete _bluetoothComponent;
    delete _nowPlayingComponent;
    delete _discComponent;
    if (_poller) delete _poller;

    _audioOutput->destroy();
}

void Application::run() {
    _app->run(*_window);
}

void Application::notify() {
    _dispatcher.emit();
}

void Application::queryDiscDB() {
    DiscDB::Disc::Builder builder;

    for (unsigned int i = 0; i < _drive.tracks(); i++) {
        builder.track(DiscDB::Track::Builder()
                      .frameOffset(_drive.lba(1 + i))
                      .build());
    }

    const DiscDB::Disc disc = builder
                              .length(_drive.seconds())
                              .calculateDiscID()
                              .build();

    _disc = DiscDB::find(disc);
}

void Application::on_notification_from_poller() {
    queryDiscDB();
    _discComponent->set_disc(&_disc);
    _nowPlayingComponent->set_album(_disc.artist(), _disc.title());
    _nowPlayingComponent->set_state(NowPlayingComponent::State::Stopped);

    delete _poller;
    _poller = nullptr;
}

void Application::on_bluetooth_button() {
    _stack->set_visible_child(*_bluetoothBox);
    _bluetoothComponent->on_show();
}

void Application::on_bluetooth_connected() {
    Glib::signal_timeout().connect([this]() {
        static int attempts = 5;

        if (attempts-- > 0) {
            _audioOutput->restart();
            if (!_audioOutput->isDefault()) {
                _bluetoothComponent->on_device_initialization_complete(true);
                attempts = 5;
                return false;
            }

            return true;
        }

        _bluetoothComponent->on_device_initialization_complete(false);
        attempts = 5;
        return false;
    }, 1000);
}

void Application::on_bluetooth_done() {
    _bluetoothComponent->on_hide();
    _stack->set_visible_child(*_playerBox);
}

void Application::on_track_selected(unsigned int track) {
    play(track);
}

void Application::on_button(const NowPlayingComponent::Button button) {
    switch (button) {
        case NowPlayingComponent::Button::Previous:
            play(_track - 1);
            break;
        case NowPlayingComponent::Button::PlayPause:
            if (_nowPlayingComponent->get_state() == NowPlayingComponent::State::Playing) {
                pause();
            } else {
                play();
            }
            break;
        case NowPlayingComponent::Button::Stop:
            stop();
            break;
        case NowPlayingComponent::Button::Next:
            play(_track + 1);
            break;
    }
}

bool Application::on_timeout() {
    if (_drive.done()) {
        eject();
    } else {
        if (_track != _drive.track()) {
            _track = _drive.track();
            _discComponent->set_selection(_track);
            _nowPlayingComponent->set_track(_disc, _track, _track == 1, _track == _disc.tracks().size());
        }
        _nowPlayingComponent->set_seconds(_drive.elapsed());
    }

    return true;
}

void Application::on_shutdown_button() {
    _systemdProxy->call_sync("PowerOff", Glib::VariantContainerBase::create_tuple(Glib::Variant<bool>::create(false)));
}

void Application::play(unsigned int track) {
    if (_nowPlayingComponent->get_state() == NowPlayingComponent::State::Playing)
        stop();

    _track = track;

    _drive.track(track);
    _discComponent->set_selection(track);
    _nowPlayingComponent->set_track(_disc, _track, track == 1, track == _disc.tracks().size());
    play();
}

void Application::play() {
    if (_track == 0) {
        play(1);
    } else {
        _timerConnection = Glib::signal_timeout().connect(sigc::mem_fun(this, &Application::on_timeout), 250);
        _audioOutput->start();
        _nowPlayingComponent->set_state(NowPlayingComponent::State::Playing);
    }
}

void Application::pause() {
    _timerConnection.disconnect();
    _audioOutput->stop();
    _nowPlayingComponent->set_state(NowPlayingComponent::State::Paused);
}

void Application::stop() {
    _track = 0;

    _timerConnection.disconnect();
    _audioOutput->stop();
    _discComponent->clear_selection();
    _nowPlayingComponent->set_state(NowPlayingComponent::State::Stopped);
}

void Application::eject() {
    if (_nowPlayingComponent->get_state() == NowPlayingComponent::State::Playing)
        stop();

    _discComponent->set_disc(nullptr);
    _nowPlayingComponent->set_state(NowPlayingComponent::State::Disabled);
    _drive.eject();

    if (!_poller)
        _poller = new Poller(*this);
}

Application::Poller::Poller(Application& app)
    : _app(app)
    , _exit(false)
    , _thread(&Application::Poller::loop, this) {

}

Application::Poller::~Poller() {
    _exit_lock.lock();
    _exit = true;
    _exit_lock.unlock();

    _thread.join();
}

void Application::Poller::loop() {
    while (true) {
        _exit_lock.lock();
        bool exit = _exit;
        _exit_lock.unlock();

        if (exit) return;

        if (_app._drive.present()) {
            _app.notify();
            return;
        }
    }
}
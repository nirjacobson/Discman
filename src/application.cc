#include "application.h"

Application::Application(int argc, char **argv)
    : _argc(argc)
    , _argv(argv)
    , _ripper(nullptr)
    , _audioOutput(AudioOutput<int16_t>::instance())
    , _track(0)
    {
    _audioOutput->producer(&_driveManager.discDrive());
    _audioOutput->init();

    AlbumArtProvider::instance()->init();

    _app = Gtk::Application::create("com.nirjacobson.discman");
    _app->signal_activate().connect(sigc::mem_fun(*this, &Application::on_activate));

    _builder = Gtk::Builder::create();
    _builder->add_from_resource("/ui/discman.glade");

    _stack = _builder->get_widget<Gtk::Stack>("stack");
    _bluetoothBox = _builder->get_widget<Gtk::Box>("bluetoothBox");
    _playerBox = _builder->get_widget<Gtk::Box>("playerBox");
    _albumArtBox = _builder->get_widget<Gtk::Box>("albumArtBox");
    _bluetoothButton = _builder->get_widget<Gtk::Button>("bluetoothButton");
    _shutdownButton = _builder->get_widget<Gtk::Button>("shutdownButton");
    _bluetoothButton->signal_clicked().connect(sigc::mem_fun(*this, &Application::on_bluetooth_button));

    _discComponent = new DiscComponent(_driveManager, _builder);
    _discComponent->set_disc(nullptr);
    _discComponent->signal_eject_requested().connect(sigc::mem_fun(*this, &Application::eject));
    _discComponent->signal_rip_requested().connect(sigc::mem_fun(*this, &Application::rip));
    _discComponent->signal_track_selected().connect(sigc::mem_fun(*this, &Application::on_track_selected));

    _nowPlayingComponent = new NowPlayingComponent(_builder);
    _nowPlayingComponent->signal_button().connect(sigc::mem_fun(*this, &Application::on_button));
    _nowPlayingComponent->signal_albumart().connect(sigc::mem_fun(*this, &Application::on_albumart_button));

    _albumArtComponent = new AlbumArtComponent(_builder);
    _albumArtComponent->signal_done().connect(sigc::mem_fun(*this, &Application::on_albumart_done));
    _albumArtComponent->signal_art().connect(sigc::mem_fun(*this, &Application::on_albumart_art));

    _bluetoothComponent = new BluetoothComponent(_builder);
    _bluetoothComponent->signal_connected().connect(sigc::mem_fun(*this, &Application::on_bluetooth_connected));
    _bluetoothComponent->signal_done().connect(sigc::mem_fun(*this, &Application::on_bluetooth_done));

    _shutdownButton->signal_clicked().connect(sigc::mem_fun(*this, &Application::on_shutdown_button));

    _window = _builder->get_widget<Gtk::Window>("window");
    _window->fullscreen();

    _driveManager.signal_inserted().connect(sigc::mem_fun(*this, &Application::on_insert));
    _driveManager.signal_ejected().connect(sigc::mem_fun(*this, &Application::on_eject));

    _systemdProxy = Gio::DBus::Proxy::create_for_bus_sync(
                        Gio::DBus::BusType::SYSTEM,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager");
}

Application::~Application() {
    delete _playerBox;
    delete _bluetoothBox;
    delete _albumArtBox;
    delete _shutdownButton;
    delete _bluetoothButton;
    delete _stack;
    delete _window;
    delete _bluetoothComponent;
    delete _albumArtComponent;
    delete _nowPlayingComponent;
    delete _discComponent;

    _audioOutput->destroy();
}

void Application::run() {
    _app->run(_argc, _argv);
}

void Application::on_insert(DriveManager::Drive drive) {
    if (drive == DriveManager::Drive::DISC) {
        queryDiscDB();
        _discComponent->set_disc(&_disc);

        std::vector<AlbumArtProvider::AlbumArt> arts = AlbumArtProvider::instance()->album_art(_disc.artist(), _disc.title(), AlbumArtComponent::ART_SIZE, AlbumArtComponent::ART_SIZE);

        _albumArtComponent->set_albumarts(arts, _window->get_width());
        _nowPlayingComponent->set_album(_albumArtURL = arts[0].url);
        _nowPlayingComponent->set_state(NowPlayingComponent::State::Stopped);

        _discComponent->show_double_eject_button(_driveManager.isRemovablePresent());

        if (_driveManager.isRemovablePresent()) {
        }
    } else if (drive == DriveManager::Drive::REMOVABLE) {
        _discComponent->show_ipod_button(true);

        _discComponent->show_double_eject_button(_driveManager.isDiscPresent());
        _discComponent->enable_ipod_button(_driveManager.isDiscPresent());
    }
};


void Application::on_eject(DriveManager::Drive drive) {
    _discComponent->show_double_eject_button(false);
    _discComponent->enable_eject_button(_driveManager.isDiscPresent() || _driveManager.isRemovablePresent());

    if (drive == DriveManager::Drive::DISC) {
        _discComponent->set_disc(nullptr);
        _nowPlayingComponent->set_state(NowPlayingComponent::State::Disabled);

        if (_driveManager.isRemovablePresent()) {
            _discComponent->enable_ipod_button(false);
        }
    } else if (drive == DriveManager::Drive::REMOVABLE) {
        _discComponent->show_ipod_button(false);
    }
};

void Application::queryDiscDB() {
    DiscDB::Disc::Builder builder;

    for (unsigned int i = 0; i < _driveManager.discDrive().tracks(); i++) {
        builder.track(DiscDB::Track::Builder()
                      .frameOffset(_driveManager.discDrive().lba(1 + i))
                      .build());
    }

    const DiscDB::Disc disc = builder
                              .length(_driveManager.discDrive().seconds())
                              .calculateDiscID()
                              .build();

    _disc = DiscDB::find(disc);
}

void Application::on_activate() {
    _app->add_window(*_window);
    _window->show();
}

void Application::on_bluetooth_button() {
    _stack->set_visible_child(*_bluetoothBox);
    _bluetoothComponent->on_show();
}

void Application::on_albumart_button() {
    _stack->set_visible_child(*_albumArtBox);
}

void Application::on_bluetooth_connected() {
    Glib::signal_timeout().connect([this]() {
        _audioOutput->restart();
        _bluetoothComponent->on_device_initialization_complete(true);
        return false;
    }, 1000);
}

void Application::on_bluetooth_done() {
    _bluetoothComponent->on_hide();
    _stack->set_visible_child(*_playerBox);
}

void Application::on_albumart_done() {
    _stack->set_visible_child(*_playerBox);
}

void Application::on_albumart_art(const std::string url) {
    _nowPlayingComponent->set_album(_albumArtURL = url);
    on_albumart_done();
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
    if (_driveManager.discDrive().done()) {
        eject(DriveManager::Drive::DISC);
    } else {
        if (_track != _driveManager.discDrive().track()) {
            _track = _driveManager.discDrive().track();
            _discComponent->set_selection(_track);
            _nowPlayingComponent->set_track(_disc, _track, _track == 1, _track == _disc.tracks().size());
        }
        _nowPlayingComponent->set_seconds(_driveManager.discDrive().elapsed());
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

    _driveManager.discDrive().track(track);
    _discComponent->set_selection(track);
    _nowPlayingComponent->set_track(_disc, _track, track == 1, track == _disc.tracks().size());
    play();
}

void Application::play() {
    if (_track == 0) {
        play(1);
    } else {
        _timerConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Application::on_timeout), 250);
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

void Application::eject(const DriveManager::Drive drive) {
    if (drive == DriveManager::Drive::DISC) {
        if (_nowPlayingComponent->get_state() == NowPlayingComponent::State::Playing) {
            stop();
        }
    }

    _driveManager.eject(drive);
}

void Application::rip(unsigned int track) {
    stop();

    _discComponent->enable_ipod_button(false);
    _discComponent->show_progress(true);

    _ripper = new CDRipper(_driveManager.discDrive(), _disc, _albumArtURL, _driveManager.removable().mountPoint());

    _ripper->signal_track_progress().connect(sigc::mem_fun(*this, &Application::on_track_progress));
    _ripper->signal_done().connect(sigc::mem_fun(*this, &Application::on_rip_done));

    _driveManager.discDrive().resize_buffer(CDDrive::BUFFER_SIZE_RIPPING);

    if (track == 0) {
        _ripper->rip();
    } else {
        _ripper->rip(track);
    }
}

void Application::on_track_progress(const unsigned track, const unsigned progress) {
    _discComponent->update_track_progress(track, progress);
}

void Application::on_rip_done() {
    _discComponent->enable_ipod_button(true);

    delete _ripper;

    _driveManager.discDrive().resize_buffer(CDDrive::BUFFER_SIZE_PLAYING);
}

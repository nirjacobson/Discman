/**
 * @file application.cc
 * @author Nir Jacobson
 * @date 2026-04-07
 */

#include "application.h"

Application::Application(int argc, char **argv)
    : _argc(argc)
    , _argv(argv)
    , _ripper(nullptr)
    , _audio_output(AudioOutput<int16_t>::instance())
    , _track(0)
    , _net_iface(std::getenv("DISCMAN_NET_IFACE"))
    , _screensaver_timeout(std::stoi(std::getenv("DISCMAN_SCREENSAVER_TIMEOUT")))
    , _last_click(std::chrono::system_clock::now())
    , _ip_address(get_ip_address())
{
    _audio_output->producer(&_drive_manager.disc_drive());
    _audio_output->init();

    AlbumArtProvider::instance()->init();

    _app = Gtk::Application::create("com.nirjacobson.discman");
    _app->signal_activate().connect(sigc::mem_fun(*this, &Application::on_activate));

    _builder = Gtk::Builder::create();
    _builder->add_from_resource("/ui/discman.glade");

    _stack = _builder->get_widget<Gtk::Stack>("stack");
    _bluetooth_box = _builder->get_widget<Gtk::Box>("bluetoothBox");
    _player_box = _builder->get_widget<Gtk::Box>("playerBox");
    _album_art_box = _builder->get_widget<Gtk::Box>("albumArtBox");
    _screensaver_box = _builder->get_widget<Gtk::Box>("screensaverBox");
    _shutdown_button = _builder->get_widget<Gtk::Button>("shutdownButton");

    _disc_component = new DiscComponent(_drive_manager, _builder);
    _disc_component->set_disc(nullptr);
    _disc_component->signal_eject_requested().connect(sigc::mem_fun(*this, &Application::eject));
    _disc_component->signal_rip_requested().connect(sigc::mem_fun(*this, &Application::rip));
    _disc_component->signal_track_selected().connect(sigc::mem_fun(*this, &Application::on_track_selected));

    _now_playing_component = new NowPlayingComponent(_builder);
    _now_playing_component->signal_button().connect(sigc::mem_fun(*this, &Application::on_button));
    _now_playing_component->signal_album_art().connect(sigc::mem_fun(*this, &Application::on_album_art_button));

    _album_art_component = new AlbumArtComponent(_builder);
    _album_art_component->signal_done().connect(sigc::mem_fun(*this, &Application::on_album_art_done));
    _album_art_component->signal_art().connect(sigc::mem_fun(*this, &Application::on_album_art_art));

    _bluetooth_component = new BluetoothComponent(_builder);
    _bluetooth_component->signal_connected().connect(sigc::mem_fun(*this, &Application::on_bluetooth_connected));
    _bluetooth_component->signal_done().connect(sigc::mem_fun(*this, &Application::on_bluetooth_done));
    _bluetooth_component->signal_button().connect(sigc::mem_fun(*this, &Application::on_bluetooth_button));

    _shutdown_button->signal_clicked().connect(sigc::mem_fun(*this, &Application::on_shutdown_button));

    _window = _builder->get_widget<Gtk::Window>("window");

    _drive_manager.signal_inserted().connect(sigc::mem_fun(*this, &Application::on_insert));
    _drive_manager.signal_ejected().connect(sigc::mem_fun(*this, &Application::on_eject));

    _time_ip_address_label = _builder->get_widget<Gtk::Label>("timeIpAddressLabel");

    _mouse.signal_clicked().connect(sigc::mem_fun(*this, &Application::on_click));

    if (_screensaver_timeout > 0) {
        _timer_screensaver_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Application::on_timeout_screensaver), 250);
    }


    _systemd_proxy = Gio::DBus::Proxy::create_for_bus_sync(
                         Gio::DBus::BusType::SYSTEM,
                         "org.freedesktop.login1",
                         "/org/freedesktop/login1",
                         "org.freedesktop.login1.Manager");
}

Application::~Application() {
    delete _player_box;
    delete _bluetooth_box;
    delete _album_art_box;
    delete _shutdown_button;
    delete _stack;
    delete _window;
    delete _bluetooth_component;
    delete _album_art_component;
    delete _now_playing_component;
    delete _disc_component;

    _audio_output->destroy();

    AlbumArtProvider::destroy();
}

void Application::run() {
    _app->run(_argc, _argv);
}

std::string Application::get_ip_address() {
    int pipe1[2];
    int pipe2[2];
    pid_t pid1, pid2;

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        return "";
    }

    if ((pid1 = fork()) == 0) {
        dup2(pipe1[WRITE_END], STDOUT_FILENO);
        close(pipe1[READ_END]);
        close(pipe1[WRITE_END]);

        char* argv[3] = { new char[strlen("ifconfig")+1], new char[_net_iface.length()+1], NULL };
        strcpy(argv[0], "ifconfig");
        strcpy(argv[1], _net_iface.c_str());
        execvp("ifconfig", argv);
    }

    if ((pid2 = fork()) == 0) {
        dup2(pipe1[READ_END], STDIN_FILENO);
        close(pipe1[READ_END]);
        close(pipe1[WRITE_END]);

        dup2(pipe2[WRITE_END], STDOUT_FILENO);
        close(pipe2[WRITE_END]);
        close(pipe2[READ_END]);

        char* argv[3] = { new char[strlen("awk")+1], new char[strlen("NR==2 {print $2}")+1], NULL };
        strcpy(argv[0], "awk");
        strcpy(argv[1], "NR==2 {print $2}");
        execvp("awk", argv);
    }

    close(pipe1[READ_END]);
    close(pipe1[WRITE_END]);
    close(pipe2[WRITE_END]);

    std::stringstream ss;
    char buf[512];
    int bytes;
    while ((bytes = read(pipe2[READ_END], buf, 512)) > 0) {
        buf[bytes] = '\0';
        ss << buf;
    }

    std::string ip_address = ss.str();

    close(pipe2[READ_END]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return ip_address;
}

void Application::on_click() {
    if (_stack->get_visible_child() == &*_screensaver_box ) {
        _stack->set_visible_child(*_player_box);
    }
    _last_click = std::chrono::system_clock::now();
}

void Application::on_insert(DriveManager::Drive drive) {
    if (drive == DriveManager::Drive::Disc) {
        query_discdb();
        _disc_component->set_disc(&_disc);

        std::vector<AlbumArtProvider::AlbumArt> arts = AlbumArtProvider::instance()->album_art(_disc.artist(), _disc.title(), AlbumArtComponent::ART_SIZE, AlbumArtComponent::ART_SIZE);

        _album_art_component->set_album_arts(arts, _window->get_width());
        _now_playing_component->set_album(_album_art_url = arts[0].url);
        _now_playing_component->set_state(NowPlayingComponent::State::Stopped);

        _disc_component->show_double_eject_button(_drive_manager.is_removable_present());

        if (_drive_manager.is_removable_present()) {
        }
    } else if (drive == DriveManager::Drive::Removable) {
        _disc_component->show_ipod_button(true);

        _disc_component->show_double_eject_button(_drive_manager.is_disc_present());
        _disc_component->enable_ipod_button(_drive_manager.is_disc_present());
    }
};


void Application::on_eject(DriveManager::Drive drive) {
    _disc_component->show_double_eject_button(false);
    _disc_component->enable_eject_button(_drive_manager.is_disc_present() || _drive_manager.is_removable_present());

    if (drive == DriveManager::Drive::Disc) {
        _disc_component->set_disc(nullptr);
        _now_playing_component->set_state(NowPlayingComponent::State::Cleared);

        if (_drive_manager.is_removable_present()) {
            _disc_component->enable_ipod_button(false);
        }
    } else if (drive == DriveManager::Drive::Removable) {
        _disc_component->show_ipod_button(false);
    }
};

void Application::query_discdb() {
    DiscDB::Disc::Builder builder;

    for (unsigned int i = 0; i < _drive_manager.disc_drive().tracks(); i++) {
        builder.track(DiscDB::Track::Builder()
                      .frame_offset(_drive_manager.disc_drive().lba(1 + i))
                      .build());
    }

    const DiscDB::Disc disc = builder
                              .length(_drive_manager.disc_drive().seconds())
                              .calculate_disc_id()
                              .build();

    _disc = DiscDB::find(disc);
}

void Application::on_activate() {
    _app->add_window(*_window);
    _window->show();
}

void Application::on_bluetooth_button() {
    _stack->set_visible_child(*_bluetooth_box);
    _bluetooth_component->on_show();
}

void Application::on_album_art_button() {
    _stack->set_visible_child(*_album_art_box);
}

void Application::on_bluetooth_connected() {
    Glib::signal_timeout().connect([this]() {
        _audio_output->restart();
        _bluetooth_component->on_device_initialization_complete(true);
        return false;
    }, 1000);
}

void Application::on_bluetooth_done() {
    _bluetooth_component->on_hide();
    _stack->set_visible_child(*_player_box);
}

void Application::on_album_art_done() {
    _stack->set_visible_child(*_player_box);
}

void Application::on_album_art_art(const std::string url) {
    _now_playing_component->set_album(_album_art_url = url);
    on_album_art_done();
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
            if (_now_playing_component->get_state() == NowPlayingComponent::State::Playing) {
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

bool Application::on_timeout_track_state() {
    if (_drive_manager.disc_drive().done()) {
        eject(DriveManager::Drive::Disc);
    } else {
        if (_track > 0 && _track != _drive_manager.disc_drive().track()) {
            _track = _drive_manager.disc_drive().track();
            _disc_component->set_selection(_track);
            _now_playing_component->set_track(_disc, _track, _track == 1, _track == _disc.tracks().size());
        }
        _now_playing_component->set_seconds(_drive_manager.disc_drive().elapsed());
    }

    return true;
}

bool Application::on_timeout_screensaver() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    char buffer[10];
    std::strftime(buffer, sizeof(buffer), "%l:%M %p", localTime);

    _time_ip_address_label->set_text(std::string(buffer)+"\n\n"+"IP: "+_ip_address);

    int seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - _last_click).count();

    if (seconds >= _screensaver_timeout) {
        _stack->set_visible_child(*_screensaver_box);
    }

    return true;
}

void Application::on_shutdown_button() {
    _systemd_proxy->call_sync("PowerOff", Glib::VariantContainerBase::create_tuple(Glib::Variant<bool>::create(false)));
}

void Application::play(unsigned int track) {
    if (_now_playing_component->get_state() == NowPlayingComponent::State::Playing)
        stop();

    _track = track;

    _drive_manager.disc_drive().track(track);
    _disc_component->set_selection(track);
    _now_playing_component->set_track(_disc, _track, track == 1, track == _disc.tracks().size());
    play();
}

void Application::play() {
    if (_track == 0) {
        play(1);
    } else {
        _timer_track_state_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Application::on_timeout_track_state), 250);
        _audio_output->start();
        _now_playing_component->set_state(NowPlayingComponent::State::Playing);
    }
}

void Application::pause() {
    _timer_track_state_connection.disconnect();
    _audio_output->stop();
    _now_playing_component->set_state(NowPlayingComponent::State::Paused);
}

void Application::stop() {
    _track = 0;

    _timer_track_state_connection.disconnect();
    _audio_output->stop();
    _disc_component->clear_selection();
    _now_playing_component->set_state(NowPlayingComponent::State::Stopped);
}

void Application::eject(const DriveManager::Drive drive) {
    if (drive == DriveManager::Drive::Disc) {
        if (_now_playing_component->get_state() == NowPlayingComponent::State::Playing) {
            stop();
        }
    }

    _drive_manager.eject(drive);
}

void Application::rip(unsigned int track) {
    _track = 0;

    _timer_track_state_connection.disconnect();
    _audio_output->stop();
    _disc_component->clear_selection();
    _now_playing_component->set_state(NowPlayingComponent::State::Disabled);

    _disc_component->enable_ipod_button(false);
    _disc_component->show_progress(true);

    _ripper = new CDRipper(_drive_manager.disc_drive(), _disc, _album_art_url, _drive_manager.removable().mount_point());

    _ripper->signal_track_progress().connect(sigc::mem_fun(*this, &Application::on_track_progress));
    _ripper->signal_done().connect(sigc::mem_fun(*this, &Application::on_rip_done));

    _drive_manager.disc_drive().resize_buffer(CDDrive::BUFFER_SIZE_RIPPING);

    if (track == 0) {
        _ripper->rip();
    } else {
        _ripper->rip(track);
    }
}

void Application::on_track_progress(const unsigned track, const unsigned progress) {
    _disc_component->update_track_progress(track, progress);
}

void Application::on_rip_done() {
    _disc_component->enable_ipod_button(true);

    delete _ripper;

    _drive_manager.disc_drive().resize_buffer(CDDrive::BUFFER_SIZE_PLAYING);

    _track = 0;
    _timer_track_state_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Application::on_timeout_track_state), 250);
    _now_playing_component->set_state(NowPlayingComponent::State::Stopped);
}

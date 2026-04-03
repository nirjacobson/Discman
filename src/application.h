#ifndef CD_PLAYER_H
#define CD_PLAYER_H

#include <glibmm/main.h>
#include <glibmm.h>
#include <glibmm/dispatcher.h>
#include <gtkmm/application.h>
#include <gtkmm/builder.h>
#include <gtkmm/window.h>
#include <gtkmm/stack.h>
#include <gtkmm/box.h>
#include <glibmm/refptr.h>
#include <glibmm/variant.h>
#include <giomm/dbusproxy.h>

#include <iostream>
#include <mutex>
#include <libgen.h>

#include <discdb/discdb.h>

#include "cd_drive.h"
#include "cd_ripper.h"
#include "disc_component.h"
#include "now_playing_component.h"
#include "albumart_provider.h"
#include "albumart_component.h"
#include "bluetooth_component.h"
#include "audio_output.h"

class Application {

    public:

        Application(int argc, char **argv);
        ~Application();

        void run();

    private:
        class Poller {
            public:
                Poller(Application& app);
                ~Poller();

                void loop();

            private:
                Application& _app;
                bool _exit;

                std::mutex _exit_lock;
                std::thread _thread;
        };

        int _argc;
        char** _argv;

        Glib::RefPtr<Gtk::Builder> _builder;
        Glib::RefPtr<Gtk::Application> _app;
        Gtk::Window* _window;
        CDDrive _drive;
        CDRipper* _ripper;
        DiscDB::Disc _disc;
        AudioOutput<int16_t>* _audioOutput;
        unsigned int _track;
        std::string _albumArtURL;

        Glib::RefPtr<Gio::DBus::Proxy> _systemdProxy;

        Glib::Dispatcher _dispatcher;
        Poller* _poller;
        sigc::connection _timerConnection;

        Gtk::Stack* _stack;
        Gtk::Button* _bluetoothButton;
        Gtk::Box* _playerBox;
        Gtk::Box* _bluetoothBox;
        Gtk::Box* _albumArtBox;
        DiscComponent* _discComponent;
        NowPlayingComponent* _nowPlayingComponent;
        BluetoothComponent* _bluetoothComponent;
        AlbumArtComponent* _albumArtComponent;

        Gtk::Button* _shutdownButton;

        void on_activate();

        void on_notification_from_poller();
        void on_bluetooth_button();
        void on_bluetooth_connected();
        void on_bluetooth_done();
        void on_track_progress(unsigned int track, unsigned int progress);
        void on_rip_done();
        void notify();

        void on_albumart_done();
        void on_albumart_art(const std::string url);

        void on_track_selected(unsigned int track);
        void on_button(const NowPlayingComponent::Button button);
        void on_prev();
        void on_playpause();
        void on_stop();
        void on_next();
        void on_albumart_button();
        bool on_timeout();

        void on_shutdown_button();

        void play(unsigned int track);
        void play();
        void pause();
        void stop();
        void eject();

        void rip(unsigned int track);

        void queryDiscDB();

};

#endif // CD_PLAYER_H

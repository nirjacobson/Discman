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
#include "drive_manager.h"
#include "cd_ripper.h"
#include "disc_component.h"
#include "now_playing_component.h"
#include "album_art_provider.h"
#include "album_art_component.h"
#include "bluetooth_component.h"
#include "audio_output.h"

class Application {

    public:

        Application(int argc, char **argv);
        ~Application();

        void run();

    private:
        int _argc;
        char** _argv;

        Glib::RefPtr<Gtk::Builder> _builder;
        Glib::RefPtr<Gtk::Application> _app;
        Gtk::Window* _window;
        DriveManager _drive_manager;
        CDRipper* _ripper;
        DiscDB::Disc _disc;
        AudioOutput<int16_t>* _audio_output;
        unsigned int _track;
        std::string _album_art_url;

        Glib::RefPtr<Gio::DBus::Proxy> _systemd_proxy;

        sigc::connection _timer_connection;

        Gtk::Stack* _stack;
        Gtk::Button* _bluetooth_button;
        Gtk::Box* _player_box;
        Gtk::Box* _bluetooth_box;
        Gtk::Box* _album_art_box;
        DiscComponent* _disc_component;
        NowPlayingComponent* _now_playing_component;
        BluetoothComponent* _bluetooth_component;
        AlbumArtComponent* _album_art_component;

        Gtk::Button* _shutdown_button;

        void on_activate();

        void on_insert(DriveManager::Drive drive);
        void on_eject(DriveManager::Drive drive);
        void on_bluetooth_button();
        void on_bluetooth_connected();
        void on_bluetooth_done();
        void on_track_progress(unsigned int track, unsigned int progress);
        void on_rip_done();

        void on_album_art_done();
        void on_album_art_art(const std::string url);

        void on_track_selected(unsigned int track);
        void on_button(const NowPlayingComponent::Button button);
        void on_prev();
        void on_playpause();
        void on_stop();
        void on_next();
        void on_album_art_button();
        bool on_timeout();

        void on_shutdown_button();

        void play(unsigned int track);
        void play();
        void pause();
        void stop();
        void eject(const DriveManager::Drive drive);

        void rip(unsigned int track);

        void query_discdb();

};

#endif // CD_PLAYER_H

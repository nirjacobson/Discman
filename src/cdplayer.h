#ifndef CD_PLAYER_H
#define CD_PLAYER_H

#include <glibmm-2.4/glibmm.h>
#include <glibmm-2.4/glibmm/dispatcher.h>
#include <gtkmm-3.0/gtkmm/application.h>
#include <gtkmm-3.0/gtkmm/builder.h>
#include <gtkmm-3.0/gtkmm/window.h>
#include <gtkmm-3.0/gtkmm/stockid.h>

#include <iostream>
#include <mutex>

#include "cd_drive.h"
#include "disc_component.h"
#include "now_playing_component.h"
#include "discdb.h"
#include "audio_output.h"

class CDPlayer {

  public:
    CDPlayer(int argc, char **argv);
    ~CDPlayer();

    void run();

  private:
    class Poller {
      public:
        Poller(CDPlayer& cdplayer);
        ~Poller();

        void loop();

      private:
        CDPlayer& _cdplayer;
        bool _present;
        bool _exit;

        std::thread _thread;
        std::mutex _exit_lock;
    };

    Glib::RefPtr<Gtk::Builder> _builder;
    Glib::RefPtr<Gtk::Application> _app;
    Gtk::Window* _window;
    CDDrive _drive;
    DiscDB::Disc _disc;
    AudioOutput<int16_t>* _audioOutput;
    unsigned int _track;

    Glib::Dispatcher _dispatcher;
    Poller* _poller;
    sigc::connection _timerConnection;

    DiscComponent* _discComponent;
    NowPlayingComponent* _nowPlayingComponent;
    
    void on_notification_from_poller();
    void notify();

    void on_track_selected(unsigned int track);
    void on_prev();
    void on_playpause();
    void on_stop();
    void on_next();
    bool on_timeout();

    void play(unsigned int track);
    void play();
    void pause();
    void stop();
    void eject();

    void queryDiscDB();

};

#endif // CD_PLAYER_H

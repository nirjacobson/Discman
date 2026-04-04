#ifndef NOW_PLAYING_COMPONENT
#define NOW_PLAYING_COMPONENT

#include <glibmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
#include <gdkmm/pixbuf.h>
#include <sigc++/signal.h>

#include <discdb/disc.h>

#include "album_art_provider.h"

class NowPlayingComponent {

    public:

        enum State {
            Disabled,
            Playing,
            Paused,
            Stopped
        };

        enum Button {
            Previous,
            PlayPause,
            Stop,
            Next
        };

        typedef sigc::signal<void(const Button)> sig_button;
        typedef sigc::signal<void(void)> sig_album_art;

        NowPlayingComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~NowPlayingComponent();

        void set_track(const DiscDB::Disc& disc, unsigned int track, const bool first, const bool last);
        void set_state(const State state);
        State get_state() const;
        void set_seconds(const float seconds);
        void set_album(const std::string& url);

        sig_button signal_button();
        sig_album_art signal_album_art();

    private:
        State _state;

        Gtk::Button* _album_art_button;
        Gtk::Image* _album_art_image;
        Gtk::Label* _track_title_label;
        Gtk::Label* _track_artist_label;
        Gtk::Scale* _seek_scale;
        Gtk::Button* _prev_button;
        Gtk::Button* _play_pause_button;
        Gtk::Button* _stop_button;
        Gtk::Button* _next_button;

        Gtk::Image* _play_pause_image;

        sig_button _signal_button;
        sig_album_art _signal_album_art;

        void on_prev_button_clicked();
        void on_playpause_button_clicked();
        void on_stop_button_clicked();
        void on_next_button_clicked();
        void on_album_art_button_clicked();
};

#endif // NOW_PLAYING_COMPONENT

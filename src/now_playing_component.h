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
#include "last_fm.h"

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

        NowPlayingComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~NowPlayingComponent();

        void set_track(const DiscDB::Disc& disc, unsigned int track, const bool first, const bool last);
        void set_state(const State state);
        State get_state() const;
        void set_seconds(const float seconds);
        void set_album(const std::string& artist, const std::string& title);

        sig_button signal_button();

    private:
        State _state;

        Gtk::Image* _albumArtImage;
        Gtk::Label* _trackTitleLabel;
        Gtk::Label* _trackArtistLabel;
        Gtk::Scale* _seekScale;
        Gtk::Button* _prevButton;
        Gtk::Button* _playPauseButton;
        Gtk::Button* _stopButton;
        Gtk::Button* _nextButton;

        Gtk::Image* _playPauseImage;

        sig_button _signal_button;

        void on_prev_button_clicked();
        void on_playpause_button_clicked();
        void on_stop_button_clicked();
        void on_next_button_clicked();
};

#endif // NOW_PLAYING_COMPONENT

/**
 * @file now_playing_component.h
 * @author Nir Jacobson
 * @date 2026-04-08
 */

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

#include "album_art/album_art_provider.h"
#include "component.h"

/// @brief Handles the interactions with the "now playing" side of the main screen.
class NowPlayingComponent : public Component {

    public:

        /// @brief The component has several states it can be put into.
        enum State {
            Disabled,
            Cleared,
            Playing,
            Paused,
            Stopped
        };

        /// @brief Playback control button identifiers.
        enum Button {
            Previous,
            PlayPause,
            Stop,
            Next
        };

        /// @brief "Playback button clicked" signal type.
        typedef sigc::signal<void(const Button)> sig_button;

        /// @brief "Album art clicked" signal type.
        typedef sigc::signal<void(void)> sig_album_art;

        /// @brief NowPlayingComponent constructor.
        /// @param [in] builder The builder initialized with the widgets managed by this component.
        NowPlayingComponent(Glib::RefPtr<Gtk::Builder> builder);

        /// @brief NowPlayingComponent destructor.
        ~NowPlayingComponent();

        /// @brief Sets the information for the track currently playing.
        /// @param disc All of the album information.
        /// @param track 1-based index of the track.
        /// @param first Whether the track is the first on the disc.
        /// @param last Whether the track is the last on the disc.
        void set_track(const DiscDB::Disc& disc, unsigned int track, const bool first, const bool last);

        /// @brief Sets the state of the component.
        /// @param [in] state The new state to enter.
        void set_state(const State state);

        /// @brief Returns the state of the component.
        /// @return The state of the component.
        State get_state() const;

        /// @brief Sets the elapsed seconds.
        /// @param [in] seconds Fractional elapsed seconds .
        void set_seconds(const float seconds);

        /// @brief Sets the album art image.
        /// @param [in] url The URL of the album art image.
        void set_album(const std::string& url);

        sig_button signal_button();         ///< Getter for ::_signal_button.
        sig_album_art signal_album_art();   ///< Getter for ::_signal_album_art.

    private:
        State _state;                       ///< State of the component.

        Gtk::Button* _album_art_button;     ///< Button that shows the album art.
        Gtk::Image* _album_art_image;       ///< Album art image.
        Gtk::Label* _track_title_label;     ///< Track title label.
        Gtk::Label* _track_artist_label;    ///< Track artist label.
        Gtk::Scale* _seek_scale;            ///< Display for elapsed time.
        Gtk::Button* _prev_button;          ///< Previous track button.
        Gtk::Button* _play_pause_button;    ///< Play/pause button.
        Gtk::Button* _stop_button;          ///< Stop button.
        Gtk::Button* _next_button;          ///< Next track button.

        /// @brief The current image displayed on the play/pause button.
        /// It will either be a "play" icon or a "pause" icon.
        Gtk::Image* _play_pause_image;

        sig_button _signal_button;          ///< Emitted when a playback control button is clicked.
        sig_album_art _signal_album_art;    ///< Emitted when the album art button is clicked.

        void on_prev_button_clicked();      ///< Called when the previous track button is clicked.
        void on_playpause_button_clicked(); ///< Called when the play/pause button is clicked.
        void on_stop_button_clicked();      ///< Called when the stop button is clicked.
        void on_next_button_clicked();      ///< Called when the next track button is clicked.
        void on_album_art_button_clicked(); ///< Called when the album art button is clicked.
};

#endif // NOW_PLAYING_COMPONENT

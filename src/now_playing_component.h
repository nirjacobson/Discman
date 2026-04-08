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

#include "album_art_provider.h"

/// @brief Handles the interactions with the "now playing" side of the main screen
class NowPlayingComponent {

    public:

        /// @brief The component has several states it can be put into
        enum State {
            Disabled,
            Cleared,
            Playing,
            Paused,
            Stopped
        };

        /// @brief Playback control button identifiers
        enum Button {
            Previous,
            PlayPause,
            Stop,
            Next
        };

        typedef sigc::signal<void(const Button)> sig_button;
        typedef sigc::signal<void(void)> sig_album_art;

        /// @brief NowPlayingComponent constructor
        /// @param [in] builder The builder initialized with the widgets managed by this component
        NowPlayingComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~NowPlayingComponent();

        /// @brief Sets the information for the track currently playing
        /// @param disc all of the album information
        /// @param track the 1-based index of the track
        /// @param first whether the track is the first on the disc
        /// @param last whether the track is the last on the disc
        void set_track(const DiscDB::Disc& disc, unsigned int track, const bool first, const bool last);

        /// @brief Sets the state of the component
        /// @param [in] state the new state to enter
        void set_state(const State state);

        /// @brief Returns the state of the component
        /// @return the state of the component
        State get_state() const;

        /// @brief Sets the elapsed seconds
        /// @param [in] seconds fractional elapsed seconds 
        void set_seconds(const float seconds);

        /// @brief Sets the album art image
        /// @param [in] url the URL of the album art image
        void set_album(const std::string& url);

        sig_button signal_button();
        sig_album_art signal_album_art();

    private:
        State _state;                       ///< The state of the component

        Gtk::Button* _album_art_button;     ///< The button that shows the album art
        Gtk::Image* _album_art_image;       ///< The album art image
        Gtk::Label* _track_title_label;     ///< The track title label
        Gtk::Label* _track_artist_label;    ///< The track artist label
        Gtk::Scale* _seek_scale;            ///< The display for elapsed time
        Gtk::Button* _prev_button;          ///< The previous track button
        Gtk::Button* _play_pause_button;    ///< The play/pause button
        Gtk::Button* _stop_button;          ///< The stop button
        Gtk::Button* _next_button;          ///< The next track button

        /// @brief The current image displayed on the play/pause button.
        /// It will either be a "play" icon or a "pause" icon.
        Gtk::Image* _play_pause_image;

        sig_button _signal_button;          ///< Emitted when a playback control button is clicked
        sig_album_art _signal_album_art;    ///< Emitted when the album art button is clicked

        void on_prev_button_clicked();      ///< Called when the previous track button is clicked
        void on_playpause_button_clicked(); ///< Called when the play/pause button is clicked
        void on_stop_button_clicked();      ///< Called when the stop button is clicked
        void on_next_button_clicked();      ///< Called when the next track button is clicked
        void on_album_art_button_clicked(); ///< Called when the album art button is clicked
};

#endif // NOW_PLAYING_COMPONENT

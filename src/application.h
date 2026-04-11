/**
 * @file application.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

#ifndef DISCMAN_H
#define DISCMAN_H

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

#include "cd/cd_drive.h"
#include "cd/cd_ripper.h"
#include "drive_manager/drive_manager.h"
#include "album_art/album_art_provider.h"
#include "component/album_art_component.h"
#include "component/bluetooth_component.h"
#include "component/disc_component.h"
#include "component/now_playing_component.h"
#include "header_only/audio_output.h"

/// @brief Captures the Discman application global scope.
class Application {

    public:

        /// @brief Application constructor.
        /// @param [in] argc Argument count.
        /// @param [in] argv Arguments.
        Application(int argc, char **argv);

        /// @brief Application destructor.
        ~Application();

        /// @brief Runs Discman.
        void run();

    private:
        int _argc;    ///< The application argument count.
        char** _argv; ///< The application arguments.

        Glib::RefPtr<Gtk::Builder> _builder; ///< The builder initialized with the Discman UI.
        Glib::RefPtr<Gtk::Application> _app; ///< The underlying Gtk::Application.
        Gtk::Window* _window;                ///< Reference to the application window.
        DriveManager _drive_manager;         ///< Drive manager.
        CDRipper* _ripper;                   ///< CD ripper.
        DiscDB::Disc _disc;                  ///< The Disc returned by DiscDB for the inserted CD.
        AudioOutput<int16_t>* _audio_output; ///< The audio output.
        unsigned int _track;                 ///< The current 1-based track being played.
        std::string _album_art_url;          ///< The URL of the currently displayed album art.

        Glib::RefPtr<Gio::DBus::Proxy> _systemd_proxy; ///< The systemd D-Bus proxy, used for host shutdown.

        sigc::connection _timer_connection;  ///< A connection of a timer to the callback that periodically checks the current track state.

        /// @name Top-level widgets
        /// @brief The application maintains references to some top-level widgets.
        ///
        /// @{
        Gtk::Stack* _stack;             ///< Stack that switches between application screens.
        Gtk::Box* _player_box;          ///< Box holding the main application screen.
        Gtk::Box* _bluetooth_box;       ///< Box holding the Bluetooth device selection screen.
        Gtk::Box* _album_art_box;       ///< Box holding the album art selection screen.
        /// @}

        /// @name Components
        /// @brief A component is responsible for managing interactions with a subset
        /// of the widgets available on a particular application screen.
        //  Normally a component manages an entire screen.
        ///
        /// @{
        DiscComponent* _disc_component;              ///< Handles the album labels, track listing and eject/rip buttons on the main application screen.
        NowPlayingComponent* _now_playing_component; ///< Handles the left-hand side of the main application screen showing album art and current track.
        BluetoothComponent* _bluetooth_component;    ///< Handles the Bluetooth device selection screen.
        AlbumArtComponent* _album_art_component;     ///< Handles the album art selection screen.
        /// @}

        Gtk::Button* _shutdown_button;               ///< Shutdown button.

        /// @brief Called when ::_app is activated.
        void on_activate();

        /// @brief Called when a drive (disc or iPod/thumb drive) is inserted.
        /// @param [in] drive The drive inserted (disc or iPod/thumb drive).
        void on_insert(DriveManager::Drive drive);

        /// @brief Called when a drive (disc or iPod/thumb drive) is removed.
        /// @param [in] drive The drive removed (disc or iPod/thumb drive).
        void on_eject(DriveManager::Drive drive);

        void on_bluetooth_connected(); ///< Called when the user connects to a Bluetooth device.
        void on_bluetooth_done();      ///< Called when the BluetoothComponent done button is clicked.
        
        /// @brief Called when the CDRipper reports progress on a certain track.
        /// @param [in] track the 1-based index of the track.
        /// @param [in] progress the current percentage completion of the track.
        void on_track_progress(unsigned int track, unsigned int progress);

        /// @brief Called when the CDRipper reports it is done.
        void on_rip_done();

        /// @brief Called when the AlbumArtComponent done button is clicked.
        void on_album_art_done();

        /// @brief Called when album art is selected in the AlbumArtComponent.
        void on_album_art_art(const std::string url);

        /// @brief Called when a track is selected in the DiscComponent.
        /// @param [in] track the 1-based index of the track.
        void on_track_selected(unsigned int track);

        /// @brief Called when a playback control button (prev/next, play/pause, stop) is clicked.
        /// @param [in] button the button clicked.
        void on_button(const NowPlayingComponent::Button button);

        /// @name Playback control button handlers
        /// @brief Individual handlers called by on_button().
        ///
        /// @{
        void on_prev();         ///< Previous button handler.
        void on_playpause();    ///< Play/pause button handler.
        void on_stop();         ///< Stop button handler.
        void on_next();         ///< Next button handler.
        /// @}

        /// @name Playback control handlers
        /// @brief Delegates called by the control button handlers.
        ///
        /// @{
            /// @brief Play track handler (used for previous/next).
            /// @see ::on_prev(), ::on_next().
            /// @param track the 1-base index of the track to play.
            void play(unsigned int track);

            /// @brief Play handler.
            /// @see ::on_playpause().
            void play();

            /// @brief Pause handler.
            /// @see ::on_playpause().
            void pause();

            /// @brief Stop handler.
            /// @see ::on_stop().
            void stop();
        /// @}

        void on_album_art_button(); ///< AlbumArtComponent done button handler.
        void on_bluetooth_button(); ///< Bluetooth button handler.
        void on_shutdown_button();  ///< Shutdown button handler.

        bool on_timeout();          ///< Called when the track monitoring timer reaches zero.

        /// @brief Instructs the DriveManager to eject a drive.
        /// @param [in] drive The drive to eject.
        void eject(const DriveManager::Drive drive);

        /// @brief Instructs the CDRipper to begin ripping one specific track.
        /// @param [in] track The 1-based index of the track.
        void rip(unsigned int track);

        /// @brief Queries DiscDB for the full information of the disc inserted.
        void query_discdb();

};

#endif // DISCMAN_H

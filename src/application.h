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

#include "cd_drive.h"
#include "drive_manager.h"
#include "cd_ripper.h"
#include "disc_component.h"
#include "now_playing_component.h"
#include "album_art_provider.h"
#include "album_art_component.h"
#include "bluetooth_component.h"
#include "audio_output.h"

/// @brief Captures the Discman application global scope
class Application {

    public:

        /// @brief Application constructor
        /// @param argc argument count
        /// @param argv arguments
        Application(int argc, char **argv);
        ~Application();

        /// @brief Run Discman
        void run();

    private:
        int _argc;    /// The application argument count
        char** _argv; /// The application arguments

        Glib::RefPtr<Gtk::Builder> _builder; ///< The builder initialized with the Discman UI
        Glib::RefPtr<Gtk::Application> _app; ///< The underlying Gtk::Application
        Gtk::Window* _window;                ///< Reference to the application window
        DriveManager _drive_manager;         ///< Drive manager
        CDRipper* _ripper;                   ///< CD ripper
        DiscDB::Disc _disc;                  ///< The Disc returned by DiscDB for the inserted CD
        AudioOutput<int16_t>* _audio_output; ///< The audio output
        unsigned int _track;                 ///< The current 1-based track being played
        std::string _album_art_url;          ///< The URL of the currently displayed album art

        Glib::RefPtr<Gio::DBus::Proxy> _systemd_proxy; /// The systemd D-Bus proxy, used for host shutdown

        sigc::connection _timer_connection; /// A connection of a timer to the callback that periodically checks the current track state

        /// @name Top-level widgets
        /// @brief The application maintains references to some top-level widgets.
        ///
        /// @{
        Gtk::Stack* _stack;             ///< Reference to the Gtk::Stack that switches between application screens
        Gtk::Button* _bluetooth_button; ///< Reference to the button that switches to the Bluetooth device selection screen
        Gtk::Box* _player_box;          ///< Reference to the Gtk::Box holding the main application screen
        Gtk::Box* _bluetooth_box;       ///< Reference to the Gtk::Box holding the Bluetooth device selection screen
        Gtk::Box* _album_art_box;       ///< Reference to the Gtk::Box holding the album art selection screen
        /// @}

        /// @name Components
        /// @brief A component is responsible for managing interactions with a subset
        /// of the widgets available on a particular application screen.
        //  Normally a component manages an entire screen.
        ///
        /// @{
        DiscComponent* _disc_component;              ///< Handles the album labels, track listing and eject/rip buttons on the main application screen
        NowPlayingComponent* _now_playing_component; ///< Handles the left-hand side of the main application screen showing album art and current track
        BluetoothComponent* _bluetooth_component;    ///< Handles the Bluetooth device selection screen
        AlbumArtComponent* _album_art_component;     ///< Handles the album art selection screen.
        /// @}

        Gtk::Button* _shutdown_button;

        /// @brief Called when the Gtk::Application is activated
        void on_activate();

        /// @brief Called when a drive (disc or iPod/thumb drive) is inserted
        /// @param [in] drive The drive inserted (disc or iPod/thumb drive)
        void on_insert(DriveManager::Drive drive);

        /// @brief Called when a dirve (disc or iPod/thumb drive) is removed
        /// @param [in] drive The drive removed (disc or iPod/thumb drive)
        void on_eject(DriveManager::Drive drive);

        void on_bluetooth_connected(); ///< Called when the user connects to a Bluetooth device
        void on_bluetooth_done();      ///< Called when the BluetoothComponent done button is clicked
        
        /// @brief Called when the CDRipper reports progress on a certain track
        /// @param [in] track the 1-based index of the track
        /// @param [in] progress the current percentage completion of the track
        void on_track_progress(unsigned int track, unsigned int progress);

        /// @brief Called when the CDRipper reports it is done
        void on_rip_done();

        /// @brief Called when the AlbumArtComponent done button is clicked
        void on_album_art_done();

        /// @brief Called when album art is selected in the AlbumArtComponent
        void on_album_art_art(const std::string url);

        /// @brief Called when a track is selected in the DiscComponent
        /// @param [in] track the 1-based index of the track
        void on_track_selected(unsigned int track);

        /// @brief Called when a playback control button (prev/next, play/pause, stop) is clicked
        /// @param [in] button the button clicked 
        void on_button(const NowPlayingComponent::Button button);

        /// @name Playback control button handlers
        /// @brief These are individual handlers called by on_button().
        ///
        /// @{
        void on_prev();
        void on_playpause();
        void on_stop();
        void on_next();
        /// @}

        /// @name Playback control handlers
        /// @brief These are delegates called by the control button handlers
        ///
        /// @{
        void play(unsigned int track);
        void play();
        void pause();
        void stop();
        /// @}

        void on_album_art_button(); ///< AlbumArtComponent done button handler
        void on_bluetooth_button(); ///< Bluetooth button handler
        void on_shutdown_button();  ///< Shutdown button handler

        bool on_timeout();          ///< Called when the track monitoring timer reaches zero

        /// @brief Instructs the DriveManager to eject a drive
        /// @param [in] drive the drive to eject
        void eject(const DriveManager::Drive drive);

        /// @brief Instructs the CDRipper to begin ripping one specific track
        /// @param [in] track the 1-based index of the track
        void rip(unsigned int track);

        /// @brief Queries DiscDB for the full disc information of the disc inserted.
        void query_discdb();

};

#endif // DISCMAN_H

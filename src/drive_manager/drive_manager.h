/**
 * @file drive_manager.h
 * @author Nir Jacobson
 * @date 2026-04-08
 */

#ifndef DRIVE_MANAGER_H
#define DRIVE_MANAGER_H

#include <mutex>
#include <thread>
#include <regex>

#include <glibmm/dispatcher.h>
#include <sigc++/sigc++.h>
#include <udisks2cc/manager.h>

#include "cd/cd_drive.h"
#include "header_only/freedesktop.h"

/// @brief Manages the state of the disc drive and any removable (iPod) drive.
class DriveManager {
    public:

        /// @brief The two drives supported.
        enum Drive {
            Disc,
            Removable
        };

        /// @brief DriveManager constructor.
        DriveManager();

        /// @brief DriveManager destructor.
        ~DriveManager();

        /// @brief "Inserted" and "Ejected" shared signal type.
        typedef sigc::signal<void(Drive)> sig_drive;

        /// @brief Thrown when the assumption of the presence of a drive is false.
        struct NoDriveException : public std::exception {
            const char* what() const throw() {
                return "No drive is present";
            }
        };

        sig_drive signal_inserted();        ///< Getter for ::_signal_inserted.
        sig_drive signal_ejected();         ///< Getter for ::_signal_ejected.

        /// @brief Eject one of the drives.
        /// @param [in] drive Drive::Disc or Drive::Removable.
        void eject(const Drive drive);

        bool is_disc_present() const;       ///< Returns the (cached) property of disc presence.
        bool is_removable_present() const;  ///< Returns the (cached) property of iPod presence.

        UDisks2::Filesystem& removable();   ///< Returns the UDisks2::Filesystem cassociated with the iPod.
        CDDrive& disc_drive();              ///< Returns the CDDrive associated with the disc drive.

    private:
        class Poller;

        sig_drive _sig_inserted;            ///< Emitted when a drive is inserted.
        sig_drive _sig_ejected;             ///< Emitted when a drive is removed.

        CDDrive _drive;                     ///< The disc drive.

        UDisks2::Manager _udisks2;          ///< UDisks2 provides access to removable disks.
        UDisks2::Filesystem* _removable_fs; ///< The filesystem associated with the iPod.

        Poller* _poller;                    ///< When running, checks for the presence of a disc in _drive.

        Glib::Dispatcher _dispatcher;       ///< Used to samely emit _sig_inserted for the disc drive.

        std::regex _acceptable_fs_pat;      ///< Filters the filesystems reported by UDisks2 to acceptable ones.

        void on_udisks2_init();             ///< Called when UDisks2 initializes the list of preregisterd drives and filesystems.
        void on_notification_from_poller(); ///< Called when _dispatcher is notified.
        void on_cddrive_eject();            ///< Called when the disc drive successfully ejects.

        /// @brief Called when a new filesystem appears.
        /// @param [in] path The D-Bus path to the filesystem.
        void on_removable_added(const std::string& path);

        /// @brief Called when a filesystem is removed.
        /// @param [in] path The D-Bus path to the filesystem.
        void on_removable_removed(const std::string& path);

        /// @brief Called when the removable drive is mounted.
        void on_removable_mounted(const std::string&);

        /// @brief Called when the removable drive is unmounted.
        void on_removable_unmounted();
};

/// @brief The Poller uses a separate thread to poll the host for the presence of an audio CD.
class DriveManager::Poller {
    public:
        /// @brief Poller constructor.
        /// @param [in] manager The parent DriveManager.
        Poller(DriveManager& manager);

        /// @brief Poller destructor.
        ~Poller();

        void poll(); ///< The execution loop performed by the thread.

    private:
        DriveManager& _manager; ///< The parent DriveManager.
        bool _exit;             ///< Stores a request to exist poll().

        /// @brief Provides mutually exclusive access to the _exit member
        /// between the the application thread and the Poller thread.
        std::mutex _exit_lock;

        /// @brief The thread used to run poll().
        std::thread _thread;
};

#endif // DRIVE_MANAGER_H
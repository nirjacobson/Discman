#ifndef DRIVE_MANAGER_H
#define DRIVE_MANAGER_H

#include <mutex>
#include <thread>
#include <regex>

#include <glibmm/dispatcher.h>
#include <sigc++/sigc++.h>
#include <udisks2cc/manager.h>

#include "cd_drive.h"
#include "freedesktop.h"

class DriveManager {
    public:
        enum Drive {
            DISC,
            REMOVABLE
        };

        DriveManager();
        ~DriveManager();

        typedef sigc::signal<void(Drive)> sig_drive;

        struct NoDriveException : public std::exception {
            const char* what() const throw() {
                return "No drive is present";
            }
        };

        sig_drive signal_inserted();
        sig_drive signal_ejected();

        void eject(const Drive drive);

        bool isDiscPresent() const;
        bool isRemovablePresent() const;

        UDisks2::Filesystem& removable();
        CDDrive& discDrive();

    private:
        class Poller;

        sig_drive _sig_inserted;
        sig_drive _sig_ejected;

        CDDrive _drive;

        UDisks2::Manager _udisks2;
        UDisks2::Filesystem* _removableFS;

        Poller* _poller;

        Glib::Dispatcher _dispatcher;

        std::regex _acceptableFSPattern;

        void on_udisks2_init();
        void on_notification_from_poller();
        void on_cddrive_eject();

        void on_removable_added(const std::string& path);
        void on_removable_removed(const std::string& path);

        void on_removable_mounted(const std::string&);
        void on_removable_unmounted();
};


class DriveManager::Poller {
    public:
        Poller(DriveManager& manager);
        ~Poller();

        void loop();

    private:
        DriveManager& _manager;
        bool _exit;

        std::mutex _exit_lock;
        std::thread _thread;
};

#endif // DRIVE_MANAGER_H
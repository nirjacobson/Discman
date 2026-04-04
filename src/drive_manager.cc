#include "drive_manager.h"

DriveManager::DriveManager()
    : _removable_fs(nullptr)
    , _poller(new Poller(*this))
    , _acceptable_fs_pat("^.+/sd[b-z]\\d+$") {
    _dispatcher.connect(sigc::mem_fun(*this, &DriveManager::on_notification_from_poller));
    _drive.signal_eject().connect(sigc::mem_fun(*this, &DriveManager::on_cddrive_eject));
    _udisks2.signal_init().connect(sigc::mem_fun(*this, &DriveManager::on_udisks2_init));
    _udisks2.signal_drive_added().connect(sigc::mem_fun(*this, &DriveManager::on_removable_added));
    _udisks2.signal_drive_removed().connect(sigc::mem_fun(*this, &DriveManager::on_removable_removed));
}

DriveManager::~DriveManager() {
    if (_poller) delete _poller;
}

DriveManager::Poller::Poller(DriveManager& manager)
    : _manager(manager)
    , _exit(false)
    , _thread(&DriveManager::Poller::loop, this) {

}
DriveManager::Poller::~Poller() {
    _exit_lock.lock();
    _exit = true;
    _exit_lock.unlock();
    _thread.join();
}

void DriveManager::Poller::loop() {
    while (true) {
        _exit_lock.lock();
        bool exit = _exit;
        _exit_lock.unlock();

        if (exit) return;

        if (_manager._drive.present()) {
            _manager._dispatcher.emit();
            return;
        }
    }
}

void DriveManager::eject(const DriveManager::Drive drive) {
    switch (drive) {
        case DriveManager::Drive::Disc:
            _drive.eject();
            break;
        case DriveManager::Drive::Removable:
        default:
            _removable_fs->unmount();
            _udisks2.drive_for_filesystem(_removable_fs)->eject();
            break;
    }
}

void DriveManager::on_cddrive_eject() {
    _sig_ejected.emit(Disc);

    if (!_poller)
        _poller = new Poller(*this);
}

DriveManager::sig_drive DriveManager::signal_inserted() {
    return _sig_inserted;
}

DriveManager::sig_drive DriveManager::signal_ejected() {
    return _sig_ejected;
}

void DriveManager::on_udisks2_init() {
    if (!_udisks2.filesystems().empty()) {
        for (const auto& fs : _udisks2.filesystems()) {
            if (std::regex_match(fs, _acceptable_fs_pat)) {
                _removable_fs = _udisks2.filesystem(fs);
                _removable_fs->signal_mounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_mounted));
                _removable_fs->signal_unmounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_unmounted));
                _sig_inserted.emit(Removable);
                return;
            }
        }
    }
}

void DriveManager::on_notification_from_poller() {
    delete _poller;
    _poller = nullptr;

    _sig_inserted.emit(Disc);
}

void DriveManager::on_removable_added(const std::string& path) {
    if (!_removable_fs) {
        if (std::regex_match(path, _acceptable_fs_pat)) {
            _removable_fs = _udisks2.filesystem(path);
            if (!_removable_fs->mount_point().empty()) {
                _sig_inserted.emit(Removable);
            }
            _removable_fs->signal_mounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_mounted));
            _removable_fs->signal_unmounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_unmounted));
        }
    }
}

void DriveManager::on_removable_removed(const std::string& path) {
    if (_removable_fs->path() == path) {
        if (!_removable_fs->mount_point().empty()) {
            _sig_ejected.emit(Removable);
        }
        
        delete _removable_fs;
        _removable_fs = nullptr;

    }
}

void DriveManager::on_removable_mounted(const std::string&) {
    _sig_inserted.emit(Removable);
}

void DriveManager::on_removable_unmounted() {
    _sig_ejected.emit(Removable);
}

bool DriveManager::is_disc_present() const {
    return _drive.present();
}

bool DriveManager::is_removable_present() const {
    return !!_removable_fs && !_removable_fs->mount_point().empty();
}

UDisks2::Filesystem& DriveManager::removable() {
    if (!is_removable_present()) {
        throw NoDriveException();
    }

    return *_removable_fs;
}

CDDrive& DriveManager::disc_drive() {
    return _drive;
}
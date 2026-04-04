#include "drive_manager.h"

DriveManager::DriveManager()
    : _removableFS(nullptr)
    , _poller(new Poller(*this))
    , _acceptableFSPattern("^.+/sd[b-z]\\d+$") {
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
        case DriveManager::Drive::DISC:
            _drive.eject();
            break;
        case DriveManager::Drive::REMOVABLE:
        default:
            _removableFS->unmount();
            _udisks2.drive_for_filesystem(_removableFS)->eject();
            break;
    }
}

void DriveManager::on_cddrive_eject() {
    _sig_ejected.emit(DISC);

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
            if (std::regex_match(fs, _acceptableFSPattern)) {
                _removableFS = _udisks2.filesystem(fs);
                _removableFS->signal_mounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_mounted));
                _removableFS->signal_unmounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_unmounted));
                _sig_inserted.emit(REMOVABLE);
                return;
            }
        }
    }
}

void DriveManager::on_notification_from_poller() {
    delete _poller;
    _poller = nullptr;

    _sig_inserted.emit(DISC);
}

void DriveManager::on_removable_added(const std::string& path) {
    if (!_removableFS) {
        if (std::regex_match(path, _acceptableFSPattern)) {
            _removableFS = _udisks2.filesystem(path);
            if (!_removableFS->mountPoint().empty()) {
                _sig_inserted.emit(REMOVABLE);
            }
            _removableFS->signal_mounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_mounted));
            _removableFS->signal_unmounted().connect(sigc::mem_fun(*this, &DriveManager::on_removable_unmounted));
        }
    }
}

void DriveManager::on_removable_removed(const std::string& path) {
    if (_removableFS->path() == path) {
        if (!_removableFS->mountPoint().empty()) {
            _sig_ejected.emit(REMOVABLE);
        }
        
        delete _removableFS;
        _removableFS = nullptr;

    }
}

void DriveManager::on_removable_mounted(const std::string&) {
    _sig_inserted.emit(REMOVABLE);
}

void DriveManager::on_removable_unmounted() {
    _sig_ejected.emit(REMOVABLE);
}

bool DriveManager::isDiscPresent() const {
    return _drive.present();
}

bool DriveManager::isRemovablePresent() const {
    return !!_removableFS && !_removableFS->mountPoint().empty();
}

UDisks2::Filesystem& DriveManager::removable() {
    if (!isRemovablePresent()) {
        throw NoDriveException();
    }

    return *_removableFS;
}

CDDrive& DriveManager::discDrive() {
    return _drive;
}
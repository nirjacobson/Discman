#include "cd_drive.h"

CDDrive::Reader::Reader(CDDrive& drive)
    : _drive(drive)
    , _action(Idle)
    , _buffer(0)
    , _thread(&CDDrive::Reader::loop, this) {
    
    _paranoia = paranoia_init(_drive._drive);
    paranoia_modeset(_paranoia, PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP);
}

CDDrive::Reader::~Reader() {
    action(Action::Exit);
    _thread.join();

    paranoia_free(_paranoia);
}

void CDDrive::Reader::action(const Action action) {
    _action_lock.lock();
    _action = action;
    _action_lock.unlock();
}

void CDDrive::Reader::track(const int track) {
    _drive._load_lock.lock();
    paranoia_seek(_paranoia, cdda_track_firstsector(_drive._drive, track), SEEK_SET);
    _drive._load_lock.unlock();
}

void CDDrive::Reader::buffer(const int buffer) {
    _buffer = buffer;
}

CDDrive::Reader::Action CDDrive::Reader::action() {
    _action_lock.lock();
    Action action = _action;
    _action_lock.unlock();

    return action;
}

void CDDrive::Reader::load() {
    _drive._load_lock.lock();
    for (int i = 0; i < BUFFER_SECTORS; i++) {
        int16_t* sector = paranoia_read(_paranoia, NULL);
        char* start = reinterpret_cast<char*>(_drive._buffers[_buffer]);
        memcpy(start + (i * CDIO_CD_FRAMESIZE_RAW), sector, CDIO_CD_FRAMESIZE_RAW);

        Action a = action();
        if (a == Action::Exit)
            return;
    }
    _drive._load_lock.unlock();
}

void CDDrive::Reader::loop() {
    while (true) {
        Action a = action();

        switch (a) {
            case Action::Idle:
                break;
            case Action::Load:
                load();

                if (action() == Action::Exit)
                    return;

                action(Action::Idle);
                break;
            case Action::Exit:
                return;
        }
    }
}

CDDrive::Poller::Poller(CDDrive& drive)
    : _drive(drive)
    , _exit(false)
    , _thread(&CDDrive::Poller::poll, this) {

}
CDDrive::Poller::~Poller() {
    _exit_lock.lock();
    _exit = true;
    _exit_lock.unlock();
    _thread.join();
}

void CDDrive::Poller::poll() {
    while (true) {
        _exit_lock.lock();
        bool exit = _exit;
        _exit_lock.unlock();

        if (exit) return;

        if (!_drive.identify()) continue;
        _drive.open();

        return;
    }
}

CDDrive::CDDrive()
    : _drive(nullptr)
    , _reader(nullptr)
    , _poller(new Poller(*this))
    , _buffer(0)
    , _buffer_idx(0)
    , _track(0)
    , _cursor(0)
    , _end(0) {
    _buffers[0] = new int16_t[BUFFER_SAMPLES];
    _buffers[1] = new int16_t[BUFFER_SAMPLES];
}


CDDrive::~CDDrive() {
    if (_reader) delete _reader;
    delete _poller;

    delete [] _buffers[1];
    delete [] _buffers[0];
    
    if (_drive)
        cdda_close(_drive);
}

bool CDDrive::present() const {
    return _reader;
}

void CDDrive::eject() {
    if (!present()) throw NoDiscPresentException();

    delete _reader;
    _reader = nullptr;

    cdda_close(_drive);
    _drive = nullptr;
    
    cdio_eject_media_drive(_device.c_str());
    _device.clear();

    if (_poller) delete _poller;
    _poller = new Poller(*this);
}

std::vector<std::string> CDDrive::devices() {
    pid_t pid;
    int p[2];

    std::vector<std::string> ready_devices;

    pipe(p);
    pid = fork();

    if (pid == 0) {
        dup2(p[WRITE_END], STDOUT_FILENO);
        close(p[READ_END]);
        close(p[WRITE_END]);

        char* argv[2] = { const_cast<char*>("lsblk"), NULL };
        execvp("lsblk", argv);
    } else {
        close(p[WRITE_END]);

        std::stringstream ss;
        char buf[512];
        int bytes;
        while ((bytes = read(p[READ_END], buf, 512)) > 0) {
            buf[bytes] = '\0';
            ss << buf;
        }

        int status;
        close(p[READ_END]);
        waitpid(pid, &status, 0);

        std::vector<std::string> rom_devices;

        std::string str;
        std::getline(ss, str);
        while (ss) {
            std::getline(ss, str);
            std::stringstream sss(str);

            std::string name;
            std::string type;
            std::string extra;

            sss >> name
                >> extra >> extra >> extra >> extra
                >> type;

            if (type == "rom")
                rom_devices.push_back("/dev/"+name);
        }

        for (const std::string& device : rom_devices) {
            int fd = __open(device.c_str(), O_RDONLY | O_NONBLOCK);

            int result = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_NONE);

            if (result == CDS_DISC_OK)
                ready_devices.push_back(device);

            close(fd);
        }
    }

    return ready_devices;
}

bool CDDrive::identify() {
    const std::vector<std::string> ready_devices = CDDrive::devices();

    if (ready_devices.empty()) return false;

    char **ppsz_cd_drives = new char*[ready_devices.size() + 1];

    for (unsigned int i = 0; i < ready_devices.size(); i++) {
        ppsz_cd_drives[i] = new char[ready_devices[i].length() + 1];
        strcpy(ppsz_cd_drives[i], ready_devices[i].c_str());
    }
    ppsz_cd_drives[ready_devices.size()] = NULL;

    ppsz_cd_drives = cdio_get_devices_with_cap(ppsz_cd_drives, CDIO_FS_AUDIO, false);

    if (ppsz_cd_drives && *ppsz_cd_drives) {
        _device = *ppsz_cd_drives;
        _drive = cdda_identify(*ppsz_cd_drives, CDDA_MESSAGE_FORGETIT, NULL);
    } else {
        return false;
    }

    cdio_free_device_list(ppsz_cd_drives);

    if (!_drive) throw DriveErrorException();

    cdda_verbose_set(_drive, CDDA_MESSAGE_FORGETIT, CDDA_MESSAGE_FORGETIT);

    return true;
}

void CDDrive::open() {
    if (!_drive) throw NoDiscPresentException();

    if ( 0 != cdda_open(_drive) ) {
        throw new DiscErrorException();
    }

    _reader = new Reader(*this);
}

void CDDrive::update_track(const unsigned int track) {
    _track = track;

    lsn_t first_sector = cdio_cddap_track_firstsector(_drive, track);
    lsn_t last_sector = cdio_cddap_track_lastsector(_drive, track);
    int sectors = last_sector - first_sector + 1;

    _end = sectors * CDIO_CD_FRAMESIZE_RAW / BYTES_PER_SAMPLE;
}

unsigned int CDDrive::tracks() const {
    if (!present()) throw NoDiscPresentException();

    return cdio_cddap_tracks(_drive);
}

void CDDrive::track(const int track) {
    if (!present()) throw NoDiscPresentException();

    update_track(track);
    _buffer = _buffer_idx = 0;
    _cursor = 0;

    _reader->track(track);

    _load_lock.lock();
    _reader->buffer(_buffer);
    _reader->action(Reader::Action::Load);
    _load_lock.unlock();
}

unsigned int CDDrive::track() const {
    return _track;
}

float CDDrive::elapsed() {
    if (!present()) throw NoDiscPresentException();

    _cursor_lock.lock();
    int cursor = _cursor;
    _cursor_lock.unlock();

    return static_cast<float>(cursor) / (2 * SAMPLE_RATE);
}

lba_t CDDrive::lba(const track_t track) const {
    if (!present()) throw NoDiscPresentException();
    
    return cdio_get_track_lba(_drive->p_cdio, track);
}

unsigned int CDDrive::seconds() const {
    return lba(CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC;
}

int16_t CDDrive::next() {
    if (_buffer_idx == 0) {
        _load_lock.lock();
        _reader->buffer(1 - _buffer);
        _reader->action(Reader::Action::Load);
        _load_lock.unlock();
    }
    if (_cursor == _end) return 0;

    int16_t sample = _buffers[_buffer][_buffer_idx++];
    _buffer_idx %= BUFFER_SAMPLES;

    if (_buffer_idx == 0) {
        _buffer = 1 - _buffer;
    }

    _cursor_lock.lock();
    if (++_cursor == _end) {
        if (_track < tracks()) {
            update_track(_track + 1);
            _cursor = 0;
        }
    }
    _cursor_lock.unlock();

    return sample;
}

bool CDDrive::done() {
    if (!present()) throw NoDiscPresentException();

    return _cursor == _end;
}
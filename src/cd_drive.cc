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
    paranoia_seek(_paranoia, cdda_track_firstsector(_drive._drive, track), SEEK_SET);
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
    for (int i = 0; i < BUFFER_SECTORS; i++) {
        int16_t* sector = paranoia_read(_paranoia, NULL);
        char* start = reinterpret_cast<char*>(_drive._buffers[_buffer]);
        memcpy(start + (i * CDIO_CD_FRAMESIZE_RAW), sector, CDIO_CD_FRAMESIZE_RAW);

        Action a = action();
        if (a == Action::Exit)
            return;
    }
}

void CDDrive::Reader::loop() {
    while (true) {
        Action a = action();

        switch (a) {
            case Action::Idle:
                break;
            case Action::Load:
                _drive._load_lock.lock();
                load();
                _drive._load_lock.unlock();

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
    , _extent(0)
    , _cursor(0) {
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
    _buffer = _extent = _cursor = 0;

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

        char* argv[2] = { new char[6], NULL };
        strcpy(argv[0], "lsblk");
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

unsigned int CDDrive::tracks() const {
    if (!present()) throw NoDiscPresentException();

    return cdio_cddap_tracks(_drive);
}

void CDDrive::track(const int track) {
    if (!present()) throw NoDiscPresentException();

    lsn_t first_sector = cdio_cddap_track_firstsector(_drive, track);
    lsn_t last_sector = cdio_cddap_track_lastsector(_drive, track);
    int sectors = last_sector - first_sector + 1;

    _extent = sectors * CDIO_CD_FRAMESIZE_RAW / BYTES_PER_SAMPLE;
    _buffer = _cursor = 0;

    _reader->track(track);

    _load_lock.lock();
    _reader->buffer(_buffer);
    _reader->action(Reader::Action::Load);
    _load_lock.unlock();
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
    _cursor_lock.lock();
    if (_cursor == _extent) {
        _cursor_lock.unlock();
        return 0;
    }

    int cursor = _cursor++;
    _cursor_lock.unlock();

    long buffer_idx = cursor % BUFFER_SAMPLES;

    if (buffer_idx == 0) {
        _load_lock.lock();
        _reader->buffer(1 - _buffer);
        _reader->action(Reader::Action::Load);
        _load_lock.unlock();
    }

    int16_t sample = _buffers[_buffer][buffer_idx];

    if (buffer_idx == BUFFER_SAMPLES - 1) {
        _buffer = 1 - _buffer;
    }

    return sample;
}

bool CDDrive::done() {
    if (!present()) throw NoDiscPresentException();

    _cursor_lock.lock();
    bool done = _cursor == _extent;
    _cursor_lock.unlock();

    return done;
}
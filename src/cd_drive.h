#ifndef CD_DRIVE_H
#define CD_DRIVE_H

#define READ_END  0
#define WRITE_END 1

#include <stdint.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <string.h>
#include <cdio/paranoia/paranoia.h>
#include <cdio/cd_types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <climits>

#include "producer.h"

const auto __open = open;

class CDDrive : public Producer<int16_t> {

    public:

        static constexpr int SAMPLE_RATE = 44100;

        struct DriveErrorException : public std::exception {
            const char* what() const throw() {
                return "CD-ROM drive could not be identified.";
            }
        };

        struct NoDiscPresentException : public std::exception {
            const char* what() const throw() {
                return "No disc present.";
            }
        };

        struct DiscErrorException : public std::exception {
            const char* what() const throw() {
                return "Could not open disc.";
            }
        };
        
        CDDrive();
        ~CDDrive();

        bool present() const;
        void eject();
        
        unsigned int tracks() const;

        void track(const int track);
        float elapsed();

        lba_t lba(const track_t track) const;
        unsigned int seconds() const;

        int16_t next();
        bool done();

    private:

        static constexpr int BYTES_PER_SAMPLE = sizeof(int16_t);
        static constexpr int BUFFER_SECTORS   = 2048;
        static constexpr int BUFFER_SAMPLES   = (CDIO_CD_FRAMESIZE_RAW / BYTES_PER_SAMPLE * BUFFER_SECTORS);

        bool identify();
        void open();

        static std::vector<std::string> devices();

        class Reader;
        class Poller;

        std::string _device;
        cdrom_drive_t* _drive;
        bool _open;
        Reader* _reader;
        Poller* _poller;

        int16_t* _buffers[2];
        uint8_t _buffer;

        long _extent;
        long _cursor;

        std::mutex _load_lock;
        std::mutex _cursor_lock;
};

class CDDrive::Reader {
    public:
        enum Action {
            Idle,
            Load,
            Exit
        };

        Reader(CDDrive& drive);
        ~Reader();

        void action(const Action action);
        Action action();

        void track(const int track);

        void buffer(const int buffer);

        void load();
        void loop();

    private:
        CDDrive& _drive;

        Action _action;

        int _buffer;

        cdrom_paranoia_t* _paranoia;

        std::thread _thread;
        std::mutex _action_lock;
};

class CDDrive::Poller {
    public:
        Poller(CDDrive& drive);
        ~Poller();
        
        void poll();

    private:
        CDDrive& _drive;

        bool _exit;
        std::mutex _exit_lock;
        std::thread _thread;
};

#endif // CD_DRIVE_H
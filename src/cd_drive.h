/**
 * @file cd_drive.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

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
#include <sigc++/signal.h>
#include <climits>
#include <chrono>

#include "producer.h"

const auto __open = open;

/// @brief Abstracts the host's disc drive
/// @see \ref Producer/Consumer.
class CDDrive : public Producer<int16_t> {

    public:
        static constexpr int BUFFER_SIZE_PLAYING  = 2048; ///< The audio buffer size when playing, in CD audio frames
        static constexpr int BUFFER_SIZE_RIPPING  =  512; ///< The audio buffer size when ripping, in CD audio frames

        static constexpr int SAMPLE_RATE = 44100;         ///< CD audio sample rate

        typedef sigc::signal<void()> sig_eject;

        /// @brief Thrown when an audio CD was internally expected in the disc drive
        /// but could not be fully recognized.
        struct DriveErrorException : public std::exception {
            const char* what() const throw() {
                return "CD-ROM drive could not be identified.";
            }
        };

        /// @brief Called when the presumption that a disc is present is false
        struct NoDiscPresentException : public std::exception {
            const char* what() const throw() {
                return "No disc present.";
            }
        };

        /// @brief Thrown when the audio CD was fully recognized
        /// but could not be opened for reading.
        struct DiscErrorException : public std::exception {
            const char* what() const throw() {
                return "Could not open disc.";
            }
        };

        CDDrive();
        ~CDDrive();

        bool present() const; ///< Returns whether a disc is in the drive

        /// @brief Ejects the disc in the drive
        /// @throw NoDiscPresentException
        void eject();

        unsigned int tracks() const; ///< Returns the number of tracks on the disc

        void track(const int track); ///< Sets the current track to the given 1-based index
        unsigned int track() const;  ///< Returns the 1-based index of the current track

        /// @brief Returns the LBA (block address) of the start of the given track on the disc
        lba_t lba(const track_t track) const;

        float elapsed();              ///< The elapsed duration of the track in fractional seconds
        float progress();             ///< The percentage completion of the rip of the current track (0.0 - 1.0)
        unsigned int seconds() const; ///< The duration of the entire disc in whole seconds

        /// @brief Resizes the audio buffer
        /// @param [in] sectors how many CD audio frames (1 frame = 1/75 seconds) to size the buffer to 
        void resize_buffer(unsigned int sectors);

        /// @brief Provides the next audio sample from the current track.
        /// Note this is one half of an audio frame which contains a left and right sample.
        /// @return The audio sample
        int16_t next() override;

        void register_consumer(const Consumer<int16_t>* const) = delete;
        int16_t next_for_consumer(const Consumer<int16_t>* const) = delete;

        /// @brief Whether the current track has been played or ripped through
        bool done();

        sig_eject signal_eject();

    private:

        /// @brief The number of bytes in one CD audio sample (an int16_t)
        static constexpr int BYTES_PER_SAMPLE = sizeof(int16_t);

        /// @brief Emitted when the disc is successfully ejected
        sig_eject _sig_eject;

        int _bufferSectors; ///< The number of CD frames in the audio buffer
        int _bufferSamples; ///< The number of audio samples (one half of an audio frame) in the audio buffer

        bool identify();                             ///< Finds a disc drive on the host with an audio disc inside
        void open();                                 ///< Opens the disc for reading
        void update_track(const unsigned int track); ///< Sets the current disc track using 1-based indexing

        /// @brief Returns the paths of device nodes corresponding to disc drives with discs that can be opened
        /// @return A list of device node paths
        static std::vector<std::string> devices();

        class Reader;
        class Poller;

        std::string _device;   ///< The path to the device node of the disc drive
        cdrom_drive_t* _drive; ///< A cdio handle to the disc drive
        Reader* _reader;       ///< Used to read audio samples into the audio buffer
        Poller* _poller;       ///< Used to poll for the first openable disc drive

        /**
         * @brief A ring buffer of size 2.
         * One buffer has new audio samples written to it.
         * When that buffer is full, it is designated as the read buffer
         * and the write buffer begins filling immediately.
         * This facilitates streaming the audio samples from the disc at a constant rate.
         */
        int16_t* _buffers[2];

        uint8_t _buffer;          ///< The 0-based index of the buffer in the ring currently designated as the write buffer
        unsigned int _buffer_idx; ///< The 0-based index into the write buffer

        unsigned int _track;      ///< The current track being played or ripped.
        unsigned int _cursor;     ///< The 0-based index into the read buffer
        unsigned int _end;        ///< The 0-based index just past the end of the read buffer

        /// @brief Provides mutually exclusive access to the drive and audio buffers
        /// between the application thread and the CDDrive::Reader thread.
        std::mutex _load_lock;

        /// @brief Provides mutually exclusive access to the track cursor
        /// between the the application thread and the PortAudio callback thread.
        std::mutex _cursor_lock;
};

/// @brief The Reader uses a separate thread to load data into the CDDrive's ring buffer
class CDDrive::Reader {
    public:

        /// @brief Actions are taken consecutively.
        /// Setting an action on the Reader takes effect after its current action.
        enum Action {
            Idle,
            Load,
            Exit
        };

        /// @brief Reader constructor
        /// @param [in] drive The parent CDDrive
        Reader(CDDrive& drive);
        ~Reader();

        /// @brief Set the next action for the Reader.
        /// @param [in] action the action to take
        void action(const Action action);

        /// @brief Returns the current Reader action in a thread-safe way.
        /// @return the current action
        Action action();

        /// @brief Seeks to the given track on the disc using 1-based indexing
        /// @param [in] track the track to seek to 
        void track(const int track);

        /// @brief Sets the index of the buffer in the CDDrive's ring buffer to read samples into
        /// @param [in] buffer The index of the buffer (0 or 1)
        void buffer(const int buffer);

        void load(); ///< Fills the current buffer with audio samples
        void loop(); ///< The execution loop performed by the thread

    private:
        CDDrive& _drive;             ///< The parent CDDrive

        Action _action;              ///< The current or next action to be taken by the Reader

        int _buffer;                 ///< The index of the buffer in the CDDrive's ring buffer to read samples into

        cdrom_paranoia_t* _paranoia; ///< cdio handle to the sample reader

        /// @brief Provides mutually exclusive access to the _action member
        /// between the the application thread and the Reader thread.
        std::mutex _action_lock;


        /// @brief The thread used to run loop().
        std::thread _thread;
};

/// @brief The Poller uses a separate thread to poll the host for a disc drive with an audio CD
class CDDrive::Poller {
    public:
        /// @brief Poller constructor
        /// @param [in] drive The parent CDDrive
        Poller(CDDrive& drive);
        ~Poller();

        void poll(); ///< The execution loop performed by the thread

    private:
        CDDrive& _drive;       ///< The parent CDDrive

        bool _exit;            ///< Stores a request to exit poll().

        /// @brief Provides mutually exclusive access to the _exit member
        /// between the the application thread and the Poller thread.
        std::mutex _exit_lock;

        /// @brief The thread used to run poll().
        std::thread _thread;
};

#endif // CD_DRIVE_H
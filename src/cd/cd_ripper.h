/**
 * @file cd_ripper.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

#ifndef CD_RIPPER_H
#define CD_RIPPER_H

#include <thread>
#include <filesystem>
#include <iomanip>

#include <unistd.h>
#include <glibmm.h>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libswresample/swresample.h>
}

#include <sigc++/signal.h>

#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include <stb/stb_image.h>

#include <discdb/disc.h>

#include "cd/cd_drive.h"
#include "producer_consumer/consumer.h"

/// @brief Rips CDs and individual tracks to M4A files.
/// @see \ref Producer/Consumer.
class CDRipper : public Consumer<int16_t> {
    public:
        /// @brief Thrown when there is no removable volume to rip to.
        struct NoMedia : public std::exception {
            const char* what() const throw() {
                return "No removable volumes are currently mounted.";
            }
        };

        /// @brief Thrown when an error occurs mid-rip.
        struct RipperErrorException : public std::exception {
                RipperErrorException(const std::string& what)
                    : _what(what) { }

                const char* what() const throw() {
                    return _what.c_str();
                }

            private:
                std::string _what;
        };

        /// @brief "Track progress made" signal type.
        typedef sigc::signal<void(unsigned int, unsigned int)> sig_track_progress;

        /// @brief "Rip done" signal type.
        typedef sigc::signal<void(void)> sig_done;

        sig_track_progress signal_track_progress(); ///< Getter for ::_signal_track_progress.
        sig_done signal_done();                     ///< Getter for ::_signal_done.

        /// @brief CDRipper constructor.
        /// @param [in] drive       Parent CDDrive.
        /// @param [in] disc        Fully populated DiscDB::Disc describing the album.
        /// @param [in] albumArtURL URL to the currently shown album art.
        /// @param [in] mediaRoot   Path to the root of the removable volume.
        CDRipper(CDDrive& drive, const DiscDB::Disc& disc, const std::string& albumArtURL, const std::string& mediaRoot);

        /// @brief CDRipper destructor.
        ~CDRipper();

        /// @brief Rip the entire disc.
        void rip();

        /// @brief Rip a specific track on the disc.
        /// @param [in] track The 1-based track index.
        void rip(const int track);

    private:

        /// @brief Contains resources that are shared by private helper functions.
        struct RipContext {
            AVFormatContext* fmt_ctx;   ///< Format (file) context.
            AVStream* st;               ///< Audio stream.
            AVStream* sta;              ///< Video stream for album art.
            const AVCodec* codec;       ///< The M4A codec.
            AVCodecContext* c;          ///< The codec context.
            AVFrame* frame;             ///< The unit of input to ffmpeg.
            AVPacket* packet;           ///< The unit of output from ffmpeg.
            SwrContext* swr_ctx;        ///< Context for resampling from int16_t to float.
        };

        sig_track_progress _sig_track_progress;    ///< Emitted when the percentage progress ripping a track has increased.
        sig_done _sig_done;                        ///< Emiited when the rip (whole disc or single track) is done.

        CDDrive& _drive;                           ///< The parent CDDrive.
        const DiscDB::Disc& _disc;                 ///< The fully populated DiscDB::Disc describing the album.
        std::thread* _thread;                      ///< Used to execute the ripping process.
        Glib::Dispatcher _dispatcher;              ///< Used by _thread to safely emit _sig_track_progress.
        Glib::Dispatcher _dispatcher_done;         ///< Used by _thread to safely emit _sig_done.
        unsigned int _track;                       ///< The 1-based index of the track currently being ripped.
        unsigned int _progress;                    ///< The current percentage completion ripping the current track.
        std::string _media_root;                   ///< The path to the root of the removable volume.
        std::string _output_dir;                   ///< The path to the album folder on the removable volume.
        std::string _output_filename;              ///< The output filename.
        std::string _album_art_url;                ///< The URL of the album art currently shown.
        uint8_t* _album_art_image;                 ///< Raw bytes received from the _album_art_url.
        int _album_art_image_size;                 ///< The size of _album_art_image.
        AVCodecID _album_art_image_codec;          ///< Reflects whether the _album_art_image is JPEG or PNG.
        std::pair<int, int> _album_art_image_dims; ///< The dimensions of _album_art_image in pixels.

        std::string make_safe(const std::string& str) const;

        void on_notification();                    ///< Called when _dispatcher is notified.
        void on_done_notification();               ///< Called when _dispatcher_done is notified.

        void rip_helper();                         ///< Private helper for rip().
        void rip_helper(const int track);          ///< Private helper for rip(const int track).

        void ensure_output_dir();                  ///< Verifies _media_root exists and sets _output_dir.

        /// @brief Begins the ripping process.
        /// @param [in] rip_ctx Pointer to the ripping context .
        void start_rip(RipContext* rip_ctx);

        /// @brief The core ripping method interacting directly with ffmpeg.
        /// @param [in] rip_ctx    Pointer to the ripping context.
        /// @param [in] continuous Whether to continue ripping after the current track.
        void do_rip(RipContext* rip_ctx, bool continuous=true);

        /// @brief Ends the ripping process.
        /// @param rip_ctx Pointer to the ripping context.
        void end_rip(RipContext* rip_ctx);

        /// @brief Creates the ouput file, tags it and writes the M4A header.
        /// @param rip_ctx Pointer to the ripping context.
        void start_file(RipContext* rip_ctx);

        /// @brief Frees resources in the ripping context specific to the output file.
        /// @param rip_ctx Pointer to the ripping context.
        void end_file(RipContext* rip_ctx);

        /// @brief Adds textual metadata (album and track information) to the output file.
        /// @param rip_ctx Pointer to the ripping context.
        void tag_file(RipContext* rip_ctx);

        /// @brief Adds _album_art_image to the output file.
        /// @param rip_ctx Pointer to the ripping context.
        void add_file_art(RipContext* rip_ctx);
};


#endif // CD_RIPPER_H
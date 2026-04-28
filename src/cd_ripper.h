#ifndef CD_RIPPER_H
#define CD_RIPPER_H

#include <thread>
#include <filesystem>
#include <iomanip>
#include <algorithm>
#include <ranges>

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

#include "consumer.h"
#include "cd_drive.h"

class CDRipper : public Consumer<int16_t> {
    public:
        struct NoMedia : public std::exception {
            const char* what() const throw() {
                return "No USB volumes are currently mounted.";
            }
        };
        struct RipperErrorException : public std::exception {
                RipperErrorException(const std::string& what)
                    : _what(what) { }

                const char* what() const throw() {
                    return _what.c_str();
                }

            private:
                std::string _what;
        };

        typedef sigc::signal<void(unsigned int, unsigned int)> sig_track_progress;
        typedef sigc::signal<void(void)> sig_done;

        sig_track_progress signal_track_progress();
        sig_done signal_done();

        CDRipper(CDDrive& drive, const DiscDB::Disc& disc, const std::string& albumArtURL, const std::string& mediaRoot);
        ~CDRipper();

        void rip();
        void rip(const int track);

    private:

        struct RipContext {
            AVFormatContext* fmt_ctx;
            AVStream* st;
            AVStream* sta;
            const AVCodec* codec;
            AVCodecContext* c;
            AVFrame* frame;
            AVPacket* packet;
            SwrContext* swr_ctx;
        };

        sig_track_progress _sig_track_progress;
        sig_done _sig_done;

        CDDrive& _drive;
        const DiscDB::Disc& _disc;
        std::thread* _thread;
        Glib::Dispatcher _dispatcher;
        Glib::Dispatcher _dispatcher_done;
        unsigned int _track;
        unsigned int _progress;
        std::string _media_root;
        std::string _output_dir;
        std::string _output_filename;
        std::string _album_art_url;
        uint8_t* _albumArtImage;
        int _album_art_image_size;
        AVCodecID _album_art_image_codec;
        std::pair<int, int> _album_art_image_dims;

        std::string make_safe(const std::string& str) const;

        void on_notification();
        void on_done_notification();

        void rip_helper();
        void rip_helper(const int track);

        void ensure_media_root();

        void start_rip(RipContext* rip_ctx);
        void do_rip(RipContext* rip_ctx, bool continuous=true);
        void end_rip(RipContext* rip_ctx);

        void start_file(RipContext* rip_ctx);
        void end_file(RipContext* rip_ctx);
        void tag_file(RipContext* rip_ctx);
        void add_file_art(RipContext* rip_ctx);
};


#endif // CD_RIPPER_H
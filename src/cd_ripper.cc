#define STB_IMAGE_IMPLEMENTATION
#include "cd_ripper.h"

CDRipper::CDRipper(CDDrive& drive, const DiscDB::Disc& disc, const std::string& albumArtURL, const std::string& mediaRoot)
    : _drive(drive)
    , _disc(disc)
    , _thread(nullptr)
    , _track(0)
    , _progress(0)
    , _media_root(mediaRoot)
    , _album_art_url(albumArtURL)
    , _albumArtImage(nullptr)
    , _album_art_image_size(0) {
    producer(&_drive);

    _dispatcher.connect(sigc::mem_fun(*this, &CDRipper::on_notification));
    _dispatcher_done.connect(sigc::mem_fun(*this, &CDRipper::on_done_notification));
}

CDRipper::~CDRipper() {
    if (_albumArtImage) {
        av_free(_albumArtImage);
    }
    if (_thread) {
        _thread->join();
        delete _thread;
    }
}

std::string CDRipper::make_safe(const std::string& str) const {
    std::string result(str);

    const std::string illegal = "<>:\"/\\|?*";

    std::ranges::replace_if(result, [&](unsigned char c) { return illegal.find(c) != std::string::npos || std::iscntrl(c); }, '_');

    return result;
}

void CDRipper::on_notification() {
    _sig_track_progress.emit(_track, _progress);
}

void CDRipper::on_done_notification() {
    _sig_done.emit();
}

void CDRipper::ensure_media_root() {
    if (_media_root.empty()) {
        throw NoMedia();
    }

    _output_dir =
        _media_root
        + "/Music"
        + "/" + make_safe(_disc.artist())
        + "/" + make_safe(_disc.title());

    std::filesystem::create_directories(_output_dir);
}

void CDRipper::start_rip(CDRipper::RipContext* rip_ctx) {
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);

    AVPacket* packet = av_packet_alloc();

    AVChannelLayout ch_layout = AV_CHANNEL_LAYOUT_STEREO;

    SwrContext* swr_ctx = nullptr;

    swr_alloc_set_opts2(&swr_ctx,
                        &ch_layout,
                        AV_SAMPLE_FMT_FLTP,
                        44100,
                        &ch_layout,
                        AV_SAMPLE_FMT_S16,
                        44100,
                        0,
                        nullptr);

    if (swr_init(swr_ctx) < 0) {
        swr_free(&swr_ctx);
        throw RipperErrorException("Failed to initialize resampler");
    }

    *rip_ctx = (struct CDRipper::RipContext) {
        .fmt_ctx = nullptr,
        .st      = nullptr,
        .codec   = codec,
        .c       = nullptr,
        .frame   = nullptr,
        .packet  = packet,
        .swr_ctx = swr_ctx
    };


    cURLpp::Cleanup cleanup;
    cURLpp::Easy easyhandle;

    std::stringstream ss;
    easyhandle.setOpt(cURLpp::Options::Url(_album_art_url));
    easyhandle.setOpt(cURLpp::Options::WriteStream(&ss));
    easyhandle.perform();

    std::string contentType;
    cURLpp::infos::ContentType::get(easyhandle, contentType);

    if (contentType == "image/jpeg") {
        _album_art_image_codec = AV_CODEC_ID_MJPEG;
    } else if (contentType == "image/png") {
        _album_art_image_codec = AV_CODEC_ID_PNG;
    }

    std::string s = ss.str();

    _albumArtImage = (uint8_t*)av_malloc(s.size());
    memcpy(_albumArtImage, s.c_str(), s.size());

    _album_art_image_size = s.size();

    int width, height;

    unsigned char* stb_image = stbi_load_from_memory(
                                   _albumArtImage,
                                   _album_art_image_size,
                                   &width,
                                   &height,
                                   nullptr,
                                   0
                               );

    stbi_image_free(stb_image);

    _album_art_image_dims = std::make_pair(width, height);
}

void CDRipper::do_rip(CDRipper::RipContext* rip_ctx, bool continuous) {
    start_file(rip_ctx);

    uint16_t* frame_input = new uint16_t[rip_ctx->c->frame_size*2];

    while (true) {
        for (int i = 0; i < rip_ctx->c->frame_size; i++) {
            frame_input[(i*2)+0] = consume();
            frame_input[(i*2)+1] = consume();

            if (_track != _drive.track() || _drive.done()) {
                rip_ctx->frame->nb_samples = i + 1;
                break;
            }
        }

        int ret = swr_convert(rip_ctx->swr_ctx,
                              rip_ctx->frame->data,
                              rip_ctx->frame->nb_samples,
                              reinterpret_cast<const uint8_t* const *>(&frame_input),
                              rip_ctx->frame->nb_samples);
        if (ret < 0) {
            throw RipperErrorException("Error performing resample");
        }

        ret = avcodec_send_frame(rip_ctx->c, rip_ctx->frame);
        if (ret < 0) {
            throw RipperErrorException("Could not send frame to encoder");
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(rip_ctx->c, rip_ctx->packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                throw RipperErrorException("Error encoding audio frame");
                return;
            }

            av_interleaved_write_frame(rip_ctx->fmt_ctx, rip_ctx->packet);

            unsigned int progress = std::ceil(_drive.progress() * 100);
            if (_progress < progress) {
                _progress = progress;
                _dispatcher.emit();
            }
        }

        if (_track != _drive.track() || _drive.done()) {

            ret = avcodec_send_frame(rip_ctx->c, nullptr);
            if (ret < 0) {
                throw RipperErrorException("Could not send frame to encoder");
            }

            while (ret >= 0) {
                ret = avcodec_receive_packet(rip_ctx->c, rip_ctx->packet);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    throw RipperErrorException("Error encoding audio frame");
                }

                av_interleaved_write_frame(rip_ctx->fmt_ctx, rip_ctx->packet);
            }

            end_file(rip_ctx);

            _track++;
            _progress = 0;

            if (continuous) {
                if (_drive.done()) {
                    break;
                }

                start_file(rip_ctx);

                delete [] frame_input;

                frame_input = new uint16_t[rip_ctx->c->frame_size*2];
            } else {
                break;
            }
        }
    }

    delete [] frame_input;
}

void CDRipper::end_rip(CDRipper::RipContext* rip_ctx) {
    swr_free(&rip_ctx->swr_ctx);
    av_packet_free(&rip_ctx->packet);
}

void CDRipper::start_file(RipContext* rip_ctx) {
    AVCodecContext* c = avcodec_alloc_context3(rip_ctx->codec);

    c->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    c->sample_rate = 44100;
    c->sample_fmt = AV_SAMPLE_FMT_FLTP;
    c->bit_rate = 128000;
    c->time_base = {1, 44100};
    c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    avcodec_open2(c, rip_ctx->codec, nullptr);

    rip_ctx->c = c;

    AVFrame* frame = av_frame_alloc();

    frame->nb_samples     = c->frame_size;
    frame->format         = c->sample_fmt;
    frame->ch_layout      = c->ch_layout;

    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        throw RipperErrorException("Could not allocate audio data buffers");
    }

    rip_ctx->frame = frame;

    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << _track;

    _output_filename = ss.str()
                       + " " + _disc.tracks()[_track - 1].title()
                       + ".m4a";

    _output_filename = make_safe(_output_filename);

    std::string output_filename = _output_dir + "/" + _output_filename;

    rip_ctx->fmt_ctx = nullptr;
    avformat_alloc_output_context2(&rip_ctx->fmt_ctx, nullptr, "ipod", output_filename.c_str());

    tag_file(rip_ctx);

    rip_ctx->st = avformat_new_stream(rip_ctx->fmt_ctx, rip_ctx->codec);
    rip_ctx->st->time_base = {1, 44100};

    avcodec_parameters_from_context(rip_ctx->st->codecpar, rip_ctx->c);

    rip_ctx->sta = avformat_new_stream(rip_ctx->fmt_ctx, nullptr);
    rip_ctx->sta->codecpar->codec_type  = AVMEDIA_TYPE_VIDEO;
    rip_ctx->sta->codecpar->codec_id    = _album_art_image_codec;
    rip_ctx->sta->codecpar->width       = _album_art_image_dims.first;
    rip_ctx->sta->codecpar->height      = _album_art_image_dims.second;
    rip_ctx->sta->disposition          |= AV_DISPOSITION_ATTACHED_PIC;

    avio_open(&rip_ctx->fmt_ctx->pb, output_filename.c_str(), AVIO_FLAG_WRITE);
    ret = avformat_write_header(rip_ctx->fmt_ctx, nullptr);

    if (ret < 0) {
        throw RipperErrorException("Could not write file header");
    }

    add_file_art(rip_ctx);
}

void CDRipper::end_file(RipContext* rip_ctx) {
    av_write_trailer(rip_ctx->fmt_ctx);
    avio_closep(&rip_ctx->fmt_ctx->pb);
    avformat_free_context(rip_ctx->fmt_ctx);
    av_frame_free(&rip_ctx->frame);
    avcodec_free_context(&rip_ctx->c);
}

void CDRipper::rip() {
    ensure_media_root();

    if (_thread) {
        _thread->join();
        delete _thread;
    }

    _thread = new std::thread(static_cast<void(CDRipper::*)(void)>(&CDRipper::rip_helper), this);
}

void CDRipper::rip_helper() {
    _track = 1;
    _progress = 0;

    _dispatcher.emit();

    _drive.track(_track);

    RipContext rip_ctx;

    start_rip(&rip_ctx);
    do_rip(&rip_ctx);
    end_rip(&rip_ctx);

    _dispatcher_done.emit();
}

void CDRipper::rip(const int track) {
    ensure_media_root();

    if (_thread) {
        _thread->join();
        delete _thread;
    }

    _thread = new std::thread(static_cast<void(CDRipper::*)(const int)>(&CDRipper::rip_helper), this, track);
}

void CDRipper::rip_helper(const int track) {
    _track = track;
    _progress = 0;

    _dispatcher.emit();

    _drive.track(_track);

    RipContext rip_ctx;

    start_rip(&rip_ctx);
    do_rip(&rip_ctx, false);
    end_rip(&rip_ctx);

    _dispatcher_done.emit();
}

CDRipper::sig_track_progress CDRipper::signal_track_progress() {
    return _sig_track_progress;
}

CDRipper::sig_done CDRipper::signal_done() {
    return _sig_done;
}

void CDRipper::tag_file(RipContext* rip_ctx) {
    std::stringstream ss;
    ss << _disc.year();

    av_dict_set(&rip_ctx->fmt_ctx->metadata, "title",  _disc.tracks()[_track - 1].title().c_str(), 0);
    av_dict_set(&rip_ctx->fmt_ctx->metadata, "artist", _disc.artist().c_str(),                     0);
    av_dict_set(&rip_ctx->fmt_ctx->metadata, "album",  _disc.title().c_str(),                      0);
    av_dict_set(&rip_ctx->fmt_ctx->metadata, "date",   ss.str().c_str(),                           0);
    av_dict_set(&rip_ctx->fmt_ctx->metadata, "genre",  _disc.genre().c_str(),                      0);

}

void CDRipper::add_file_art(RipContext* rip_ctx) {
    uint8_t* newImage = (uint8_t*)av_memdup(_albumArtImage, _album_art_image_size);

    AVPacket* pkt = av_packet_alloc();
    int ret = av_packet_from_data(pkt, _albumArtImage, _album_art_image_size);

    if (ret != 0) {
        throw RipperErrorException("Could not allocate packet buffer");
    }

    pkt->stream_index = rip_ctx->sta->index;
    pkt->flags       |= AV_PKT_FLAG_KEY;
    av_interleaved_write_frame(rip_ctx->fmt_ctx, pkt);

    av_packet_free(&pkt);

    _albumArtImage = newImage;
}
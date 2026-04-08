/**
 * @file album_art_provider.cc
 * @author Nir Jacobson
 * @date 2026-04-07
 */

 #include "album_art_provider.h"
#include "last_fm.h"
#include "spotify.h"

AlbumArtProvider* AlbumArtProvider::_instance = nullptr;

AlbumArtProvider* AlbumArtProvider::instance() {
    if (_instance == nullptr) {
        std::vector<std::string> providers = {
            "lastfm",
            "spotify"
        };

        std::string provider = std::getenv("DISCMAN_ALBUMART_PROVIDER")
                               ? std::getenv("DISCMAN_ALBUMART_PROVIDER")
                               : "";

        std::transform(provider.begin(), provider.end(), provider.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        auto it = std::find(providers.begin(), providers.end(), provider);

        int providerIndex = std::distance(providers.begin(), it);

        switch (providerIndex) {
            case 0:
                try {
                    _instance = new LastFM;
                    break;
                } catch (std::exception&) {

                }
            default:
            case 1:
                try {
                    _instance = new Spotify;
                } catch (std::exception&) {
                    _instance = new LastFM;
                }
                break;
        }
    }

    return _instance;
}

void AlbumArtProvider::destroy() {
    if (_instance) delete _instance;
}

void AlbumArtProvider::init() {

}



AlbumArtProvider::AlbumArt AlbumArtProvider::album_art(const std::string& url, const int width, const int height) {
    cURLpp::Cleanup cleanup;
    cURLpp::Easy easyhandle;

    std::stringstream ss;
    easyhandle.setOpt(cURLpp::Options::Url(url));
    easyhandle.setOpt(cURLpp::Options::WriteStream(&ss));
    easyhandle.perform();

    std::string data = ss.str();
    Glib::RefPtr<Glib::Bytes> bytes = Glib::Bytes::create(data.c_str(), data.size());
    Glib::RefPtr<Gio::MemoryInputStream> is = Gio::MemoryInputStream::create();
    is->add_bytes(bytes);

    AlbumArtProvider::AlbumArt art = {
        .art = Gdk::Pixbuf::create_from_stream_at_scale(is, width, height, true),
        .url = url
    };

    return art;
}

std::string AlbumArtProvider::url_with_params(const std::string& url, const std::map<std::string, std::string>& params) {
    std::string new_url(url);
    if (new_url[new_url.length() - 1] != '/') new_url += '/';

    std::stringstream ss;
    ss << new_url;
    for (std::map<std::string, std::string>::const_iterator it=params.cbegin(); it!=params.cend(); ++it) {
        ss << (it == params.cbegin() ? '?' : '&');

        ss << it->first << '=' << url_encode(it->second);
    }

    return ss.str();
}

std::string AlbumArtProvider::url_encode(const std::string& input) {
    std::stringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = input.begin(), n = input.end(); i != n; ++i) {
        const std::string::value_type c = (*i);

        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        } else if (c == ' ') {
            escaped << '+';
            continue;
        }

        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}
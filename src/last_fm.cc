#include "last_fm.h"

std::string LastFM::_api_key;

LastFM::LastFM() {
    if (!std::getenv("LASTFM_API_KEY")) {
        throw AlbumArtProvider::InitializationFailed();
    }
}

void LastFM::init() {
    LastFM::_api_key = std::getenv("LASTFM_API_KEY");
}

std::vector<AlbumArtProvider::AlbumArt> LastFM::album_art(const std::string& artist, const std::string& title, const int width, const int height) {
    cURLpp::Cleanup cleanup;
    cURLpp::Easy easyhandle;

    std::string request_url = url(Method::ALBUM_GET_INFO, {
        {"artist", artist},
        {"album", title}
    });

    std::stringstream ss;
    easyhandle.setOpt(cURLpp::Options::Url(request_url));
    easyhandle.setOpt(cURLpp::Options::WriteStream(&ss));
    easyhandle.perform();

    Json::Value root;
    ss >> root;

    const Json::Value image = root["album"]["image"];

    const std::vector<std::string> sizes = {{
        "mega",
        "extralarge",
        "large",
        "medium",
        "small",
        ""
    }};

    std::vector<AlbumArtProvider::AlbumArt> albumArts;

    for (unsigned int i = 0; i < sizes.size(); i++) {
        for (unsigned int j = 0; j < image.size(); j++) {
            const Json::Value element = image[j];
            if (element["size"] == sizes[i]) {
                request_url = element["#text"].asString();
                if (!request_url.empty()) {
                    easyhandle.setOpt(cURLpp::Options::Url(request_url));

                    ss.str("");
                    easyhandle.perform();

                    std::string data = ss.str();
                    Glib::RefPtr<Glib::Bytes> bytes = Glib::Bytes::create(data.c_str(), data.size());
                    Glib::RefPtr<Gio::MemoryInputStream> is = Gio::MemoryInputStream::create();
                    is->add_bytes(bytes);

                    AlbumArtProvider::AlbumArt art = {
                        .art = Gdk::Pixbuf::create_from_stream_at_scale(is, width, height, true),
                        .url = request_url
                    };

                    albumArts.push_back(art);
                }
            }
        }
    }

    if (albumArts.empty()) {
        throw NotFoundException();
    }

    return albumArts;
}

AlbumArtProvider::AlbumArt LastFM::album_art(const std::string& url, const int width, const int height) {
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

std::string LastFM::method_name(const Method method) {
    switch (method) {
        default:
        case ALBUM_GET_INFO:
            return "album.getinfo";
    }
}

std::string LastFM::url(const Method method, const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> full_params(params);
    full_params["method"] = method_name(method);
    full_params["api_key"] = _api_key;
    full_params["format"] = "json";

    return url_with_params(BASE_URL, full_params);
}

std::string LastFM::url_with_params(const std::string& url, const std::map<std::string, std::string>& params) {
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

std::string LastFM::url_encode(const std::string& input) {
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

#include "spotify.h"

std::string Spotify::_accessToken;

void Spotify::init() {
    std::string clientId = std::getenv("SPOTIFY_CLIENT_ID");
    std::string clientSecret = std::getenv("SPOTIFY_CLIENT_SECRET");

    cURLpp::Cleanup cleanup;
    cURLpp::Easy easyhandle;

    std::string request_url = "https://accounts.spotify.com/api/token";

    std::list<std::string> headers;
    headers.push_back("Authorization: Basic "+base64_encode(clientId+":"+clientSecret));
    headers.push_back("Content-Type: application/x-www-form-urlencoded");

    std::string form = "grant_type=client_credentials";

    std::stringstream ss;
    easyhandle.setOpt(cURLpp::Options::Url(request_url));
    easyhandle.setOpt(cURLpp::Options::HttpHeader(headers));
    easyhandle.setOpt(new curlpp::options::PostFields(form));
    easyhandle.setOpt(new curlpp::options::PostFieldSize(form.size()));
    easyhandle.setOpt(cURLpp::Options::WriteStream(&ss));
    easyhandle.perform();

    Json::Value response;
    ss >> response;

    Spotify::_accessToken = response["access_token"].asString();
}

std::vector<AlbumArtProvider::AlbumArt> Spotify::album_art(const std::string& artist, const std::string& title, const int width, const int height) {
    cURLpp::Cleanup cleanup;
    cURLpp::Easy easyhandle;

    std::string request_url = url(Method::SEARCH_FOR_ITEM, {
        {"q", "artist:"+artist+" album:"+title},
        {"type", "album"},
        {"limit", "10"},
    });

    std::list<std::string> headers;
    headers.push_back("Authorization: Bearer "+_accessToken);

    std::stringstream ss;
    easyhandle.setOpt(cURLpp::Options::Url(request_url));
    easyhandle.setOpt(cURLpp::Options::HttpHeader(headers));
    easyhandle.setOpt(cURLpp::Options::WriteStream(&ss));
    easyhandle.perform();

    Json::Value root;
    ss >> root;

    std::map<int, std::vector<std::string>> urlsBySize;

    Json::Value items = root["albums"]["items"];

    for (unsigned int i = 0; i < items.size(); i++) {
        Json::Value images = items[i]["images"];

        for (unsigned int j = 0; j < images.size(); j++) {
            int size = images[j]["width"].asInt() * images[j]["height"].asInt();
            std::string url = images[j]["url"].asString();

            if (urlsBySize.contains(size)) {
                urlsBySize[size].push_back(url);
            } else {
                urlsBySize[size] = {{ url }};
            }
        }
    }

    std::vector<AlbumArt> arts;

    for (auto it = urlsBySize.rbegin(); it != urlsBySize.rend(); ++it) {
        for (const std::string& url : it->second) {
            easyhandle.setOpt(cURLpp::Options::Url(url));

            ss.str("");
            easyhandle.perform();

            std::string data = ss.str();
            Glib::RefPtr<Glib::Bytes> bytes = Glib::Bytes::create(data.c_str(), data.size());
            Glib::RefPtr<Gio::MemoryInputStream> is = Gio::MemoryInputStream::create();
            is->add_bytes(bytes);

            AlbumArtProvider::AlbumArt art = {
                .art = Gdk::Pixbuf::create_from_stream_at_scale(is, width, height, true),
                .url = url
            };

            arts.push_back(art);
        }
    }

    if (arts.empty()) {
        throw AlbumArtProvider::NotFoundException();
    }

    return arts;
}

AlbumArtProvider::AlbumArt Spotify::album_art(const std::string& url, const int width, const int height) {
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

std::string Spotify::base64_encode(const std::string& input) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_write(bio, input.c_str(), input.length());
    BIO_flush(bio);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string output(bufferPtr->data, bufferPtr->length);

    BIO_free_all(bio);

    return output;
}

std::string Spotify::method_name(const Spotify::Method method) {
    switch (method) {
        default:
        case SEARCH_FOR_ITEM:
            return "search";
    }
}

std::string Spotify::url(const Method method, const std::map<std::string, std::string>& params) {
    return url_with_params(BASE_URL+method_name(method), params);
}

std::string Spotify::url_with_params(const std::string& url, const std::map<std::string, std::string>& params) {
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

std::string Spotify::url_encode(const std::string& input) {
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

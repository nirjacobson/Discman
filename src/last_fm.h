#ifndef LAST_FM_H
#define LAST_FM_H

#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <string>
#include <map>
#include <exception>
#include <glibmm.h>
#include <giomm/memoryinputstream.h>
#include <gdkmm/pixbuf.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>

#include "albumart_provider.h"

class LastFM : public AlbumArtProvider {

    public:
        enum Method {
            ALBUM_GET_INFO
        };

        static constexpr const char* BASE_URL = "http://ws.audioscrobbler.com/2.0/";

        void init() override;

        std::vector<AlbumArt> album_art(const std::string& artist, const std::string& title, const int width, const int height) override;
        AlbumArt album_art(const std::string& url, const int width, const int height) override;

    private:
        static std::string method_name(const Method method);
        static std::string url(const Method method, const std::map<std::string, std::string>& params);
        static std::string url_with_params(const std::string& url, const std::map<std::string, std::string>& params);
        static std::string url_encode(const std::string& input);

        static std::string _apiKey;
};

#endif // LAST_FM_H
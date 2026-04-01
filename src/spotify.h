#ifndef SPOTIFY_H
#define SPOTIFY_H

#include <string>
#include <vector>
#include <iomanip>

#include <glibmm.h>
#include <giomm/memoryinputstream.h>
#include <gdkmm/pixbuf.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "albumart_provider.h"

class Spotify : public AlbumArtProvider {
    public:

        enum Method {
            SEARCH_FOR_ITEM
        };

        static constexpr const char* BASE_URL = "https://api.spotify.com/v1/";

        void init() override;

        std::vector<AlbumArt> album_art(const std::string& artist, const std::string& title, const int width, const int height) override;
        AlbumArt album_art(const std::string& url, const int width, const int height) override;

    private:
        static std::string _accessToken;

        static std::string method_name(const Method method);
        static std::string base64_encode(const std::string& input);
        static std::string url(const Method method, const std::map<std::string, std::string>& params);
        static std::string url_with_params(const std::string& url, const std::map<std::string, std::string>& params);
        static std::string url_encode(const std::string& input);

};

#endif // SPOTIFY_H
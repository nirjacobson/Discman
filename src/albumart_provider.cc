#include "albumart_provider.h"
#include "last_fm.h"
#include "spotify.h"

AlbumArtProvider* AlbumArtProvider::_instance = nullptr;

AlbumArtProvider::~AlbumArtProvider() {

}

AlbumArtProvider* AlbumArtProvider::instance() {
    if (_instance == nullptr) {
        std::vector<std::string> providers = {
            "lastfm",
            "spotify"
        };

        std::string provider = std::getenv("DISCMAN_ALBUMART_PROVIDER")
            ? std::getenv("DISCMAN_ALBUMART_PROVIDER")
            : "";

        std::transform(provider.begin(), provider.end(), provider.begin(), [](unsigned char c){ return std::tolower(c); });

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

void AlbumArtProvider::init() {

}
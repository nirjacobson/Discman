#include "albumart_provider.h"
#include "last_fm.h"
#include "spotify.h"

AlbumArtProvider* AlbumArtProvider::_instance = nullptr;

AlbumArtProvider::~AlbumArtProvider() {

}

AlbumArtProvider* AlbumArtProvider::instance() {
    if (_instance == nullptr) {
        _instance = new Spotify;
    }

    return _instance;
}

void AlbumArtProvider::init() {

}
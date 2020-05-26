#include "track.h"

const std::string& Track::artist() const {
    return _artist;
}

const std::string& Track::title() const {
    return _title;
}

unsigned int Track::length() const {
    return _length;
}

Track::Builder& Track::Builder::artist(const std::string& artist) {
    _track._artist = artist;

    return *this;
}

Track::Builder& Track::Builder::title(const std::string& title) {
    _track._title = title;

    return *this;
}

Track::Builder& Track::Builder::length(const unsigned int length) {
    _track._length = length;

    return *this;
}

Track Track::Builder::build() {
    return _track;
}

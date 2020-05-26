#include "disc.h"

const std::string& Disc::artist() const {
    return _artist;
}

const std::string& Disc::title() const {
    return _title;
}

const std::string& Disc::genre() const {
    return _genre;
}

unsigned int Disc::length() const {
    return _length;
}

unsigned int Disc::year() const {
    return _year;
}

const std::vector<Track>& Disc::tracks() const {
    return _tracks;
}

Disc::Builder& Disc::Builder::artist(const std::string& artist) {
    _disc._artist = artist;

    return *this;
}

Disc::Builder& Disc::Builder::title(const std::string& title) {
    _disc._title = title;

    return *this;
}

Disc::Builder& Disc::Builder::genre(const std::string& genre) {
    _disc._genre = genre;

    return *this;
}

Disc::Builder& Disc::Builder::length(const unsigned int length) {
    _disc._length = length;

    return *this;
}

Disc::Builder& Disc::Builder::year(const unsigned int year) {
    _disc._year = year;

    return *this;
}

Disc::Builder& Disc::Builder::track(const Track& track) {
    _disc._tracks.push_back(track);

    return *this;
}

Disc Disc::Builder::build() {
    return _disc;
}


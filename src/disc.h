#ifndef DISC_H
#define DISC_H

#include <string>
#include <vector>

#include "track.h"

class Disc {
    public:
        const std::string& artist() const;
        const std::string& title() const;
        const std::string& genre() const;
        unsigned int length() const;
        unsigned int year() const;
        const std::vector<Track>& tracks() const;

        class Builder;

    private:
        std::string _artist;
        std::string _title;
        std::string _genre;
        unsigned int _length;
        unsigned int _year;
        std::vector<Track> _tracks;
};

class Disc::Builder {

    public:
        Builder& artist(const std::string& artist);
        Builder& title(const std::string& title);
        Builder& genre(const std::string& title);
        Builder& length(const unsigned int length);
        Builder& year(const unsigned int year);
        Builder& track(const Track& track);

        Disc build();

    private:
        Disc _disc;
};

#endif // DISC_H

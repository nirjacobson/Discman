#ifndef TRACK_H
#define TRACK_H

#include <string>

class Track {
    public:
        const std::string& artist() const;
        const std::string& title() const;
        unsigned int length() const;

        class Builder;

    private:
        std::string _artist;
        std::string _title;
        unsigned int _length;
};

class Track::Builder {

    public:
        Builder& artist(const std::string& artist);
        Builder& title(const std::string& title);
        Builder& length(const unsigned int length);

        Track build();

    private:
        Track _track;
};

#endif // TRACK_H

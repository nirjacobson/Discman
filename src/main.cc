#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include "cddb.h"
#include "cd_drive.h"
#include "audio_output.h"

char menu() {
    std::cout << "CD Player" << std::endl
              << "=========" << std::endl
              << "i - Print CD info" << std::endl
              << "t - Play track" << std::endl
              << "e - Eject disc" << std::endl
              << "q - Quit" << std::endl
              << std::endl
              << "> ";

    std::string line;
    getline(std::cin, line);

    return line[0];
}

void cd_info(CDDrive& cd) {
    try {
        CDDB cddb;
        Disc disc = cddb.disc(cd);

        std::stringstream ss;
        unsigned int h = disc.length() / 3600;
        unsigned int m = (disc.length() - (h * 3600)) % 60;
        unsigned int s = (disc.length() - (h * 3600) - (m * 60)) % 60;
        ss << std::setw(2) << std::setfill('0') << h << ":"
        << std::setw(2) << std::setfill('0') << m << ":"
        << std::setw(2) << std::setfill('0') << s;

        std::cout << "Artist: " << disc.artist() << std::endl
                << "Title:  " << disc.title() << std::endl
                << "Genre:  " << disc.genre() << std::endl
                << "Year:   " << disc.year() << std::endl
                << "Length: " << ss.str() << std::endl
                << std::endl;

        std::cout << " Track" << " | "
                << std::left << std::setw(36) << std::setfill(' ') << "Title" << " | "
                << "Length"
                << std::endl;

        std::cout << "---------"
                << std::setw(36) << std::setfill('-') << ""
                << "----------"
                << std::endl;

        for (unsigned int i = 0; i < disc.tracks().size(); i++) {
            std::stringstream ss;
            ss << std::setw(2) << std::setfill('0') << disc.tracks()[i].length() / 60 << ":"
            << std::setw(2) << std::setfill('0') << disc.tracks()[i].length() % 60;

            std::cout << std::right << std::setw(6) << std::setfill(' ') << i + 1 << " | "
                    << std::left << std::setw(36) << std::setfill(' ') << disc.tracks()[i].title().substr(0, 36) << " | "
                    << std::right << std::setw(6) << std::setfill(' ') << ss.str()
                    << std::endl;
        }

        std::cout << std::endl;
    } catch (const CDDB::NoResultsFoundException& e) {
        std::cout << "No CDDB results found!" << std::endl << std::endl;
    }
}

void play_track(CDDrive& cd, AudioOutput<int16_t>* const audioOutput) {
    int tracks = cd.tracks();
    std::cout << "Which track? [1-" << tracks <<"] ";
    std::string line;
    getline(std::cin, line);
    std::stringstream ss(line);
    int track;
    ss >> track;
    
    cd.track(track);

    audioOutput->start();

    while (!cd.done()) {
        int seconds = static_cast<int>(cd.elapsed());
        std::cout << '\r'
                  << std::setfill('0') << std::setw(2) << (seconds / 60)
                  << ':'
                  << std::setfill('0') << std::setw(2) << (seconds % 60);
    }
    std::cout << std::endl;

    audioOutput->stop();
}

int main() {
    CDDrive cd;
    AudioOutput<int16_t>* audioOutput = AudioOutput<int16_t>::instance();
    audioOutput->producer(&cd);
    audioOutput->init();

    bool run = true;
    while (run) {
        char option = menu();

        try {
            switch (option) {
                case 'i':
                    cd_info(cd);
                    break;
                case 't':
                    play_track(cd, audioOutput);
                    break;
                case 'e':
                    cd.eject();
                    break;
                case 'q':
                    run = false;
                    break;
                default:
                    break;
            }
        } catch (const CDDrive::NoDiscPresentException& e) {
            std::cout << "No disc present!" << std::endl << std::endl;
        }

        if (!run)
            break;
    }

    audioOutput->destroy();
    libcddb_shutdown();
}
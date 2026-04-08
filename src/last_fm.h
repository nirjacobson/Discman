/**
 * @file last_fm.h
 * @author Nir Jacobson
 * @date 2026-04-08
 */

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

#include "album_art_provider.h"

/// @brief An interface to last.fm for retrieving album art
class LastFM : public AlbumArtProvider {

    public:
        /// @brief last.fm API methods
        enum Method {
            AlbumGetInfo
        };

        /// @brief The last.fm API base URL
        static constexpr const char* BASE_URL = "http://ws.audioscrobbler.com/2.0/";

        LastFM();

        /// @brief Retrieves the last.fm API key as an environment variable
        void init() override;

        /// @brief Retrieves all album art matching the artist and title
        /// @param [in] artist the album artist
        /// @param [in] title  the album title
        /// @param [in] width  the width of the returned images
        /// @param [in] height the height of the returned images
        /// @return a list of album art images
        std::vector<AlbumArt> album_art(const std::string& artist, const std::string& title, const int width, const int height) override;

    private:
        /// @brief Conversion method to convert a LastFM::Method to a string
        /// @param [in] method the last.fm method
        /// @return the method as a string
        static std::string method_name(const Method method);

        /// @brief Calls url_with_params() after adding additional parameters in params.
        /// These additional parameters set the last.fm API key and method name.
        /// @param [in] method The last.fm method
        /// @param [in] params method parameters only
        /// @return a last.fm API endpoint URL
        static std::string url(const Method method, const std::map<std::string, std::string>& params);

        static std::string _api_key;
};

#endif // LAST_FM_H
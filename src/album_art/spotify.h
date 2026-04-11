/**
 * @file spotify.h
 * @author Nir Jacobson
 * @date 2026-04-08
 */

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
#include <curlpp/Infos.hpp>
#include <curlpp/cURLpp.hpp>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "album_art_provider.h"

/// @brief An interface to Spotify for retrieving album art.
class Spotify : public AlbumArtProvider {
    public:

        /// @brief Spotify API methods.
        enum Method {
            SearchForItem
        };

        /// @brief The Spotify API base URL.
        static constexpr const char* BASE_URL = "https://api.spotify.com/v1/";

        /// @brief Spotify constructor.
        Spotify();

        /// @brief Retrieves the Spotify client ID and secret as environment variables
        //  and uses them to retrieve an API key.
        void init() override;

        /// @brief Retrieves all album art matching the artist and title.
        /// @param [in] artist Album artist.
        /// @param [in] title  Album title.
        /// @param [in] width  Width of the returned images.
        /// @param [in] height Height of the returned images.
        /// @return List of album art images.
        std::vector<AlbumArt> album_art(const std::string& artist, const std::string& title, const int width, const int height) override;

    private:
        /// @brief The Spotify API key retrieved.
        static std::string _accessToken;

        /// @brief Conversion method to convert a Spotify::Method to a string.
        /// @param [in] method Spotify method.
        /// @return method as a string.
        static std::string method_name(const Method method);

        /// @brief Encodes a string into base64. Used to encode an HTTP authorization header.
        /// @param [in] input An arbitrary string.
        /// @return The base64-encoded input.
        static std::string base64_encode(const std::string& input);

        /// @brief Calls url_with_params() with the Spotify API method URL.
        /// @param [in] method Spotify method.
        /// @param [in] params Method parameters only.
        /// @return Spotify API endpoint URL.
        static std::string url(const Method method, const std::map<std::string, std::string>& params);
};

#endif // SPOTIFY_H
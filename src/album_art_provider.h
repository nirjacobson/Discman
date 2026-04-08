/**
 * @file album_art_provider.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

#ifndef ALBUM_ART_PROVIDER_H
#define ALBUM_ART_PROVIDER_H

#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include <glibmm.h>
#include <giomm/memoryinputstream.h>
#include <gdkmm/pixbuf.h>

/// @brief Abstracts web services that provide album art images
class AlbumArtProvider {
    public:

        /// @brief Thrown when the provider cannot be reached
        struct InitializationFailed : public std::exception {
            const char* what() const throw() {
                return "Album art provider could not initialize. Please check your environment variables.";
            }
        };

        /// @brief Thrown when the provider cannot find album art images
        struct NotFoundException : public std::exception {
            const char* what() const throw() {
                return "Resource could not be found.";
            }
        };

        /// @brief An album art image and its URL
        struct AlbumArt {
            Glib::RefPtr<Gdk::Pixbuf> art;
            std::string url;
        };

        /// @brief Factory method to return the album art provider.
        /// It may be called one or more times.
        /// Be sure to destroy() the instance in order to avoid leaks.
        /// It is suggested to do so in the teardown of the first caller.
        /// @return The provider that has been selected via configuration file
        static AlbumArtProvider* instance();

        /// @brief Destroy the album art provider
        static void destroy();

        /// @brief Initialize the album art provider
        /// (i.e. retrieve and verify an API key)
        virtual void init();

        /// @brief Return all album art matching the criteria, at a specific width and height
        /// @param [in] artist the album artist 
        /// @param [in] title  the album title
        /// @param [in] width  the album art image width
        /// @param [in] height the album art image height
        /// @return A list of album art images
        virtual std::vector<AlbumArt> album_art(const std::string& artist, const std::string& title, const int width, const int height) = 0;

        /// @brief             Retrieves the album art image at a given URL
        /// @param [in] url    the URL of the album art image to retrieve
        /// @param [in] width  the width of the returned image
        /// @param [in] height the height of the returned image
        /// @return            an album art image
        AlbumArt album_art(const std::string& url, const int width, const int height);

        /// @brief Returns the URL of the API endpoint with the given parameters encoded as query parameters.
        /// @param [in] url an API endpoint. This usually corresponds to an API method.
        /// @param [in] params the method parameters as key-value pairs
        /// @return an API endpoint URL
        static std::string url_with_params(const std::string& url, const std::map<std::string, std::string>& params);

        /// @brief Encodes an arbitrary string into a query parameter value
        /// @param [in] input an arbitrary string
        /// @return the URL encoded input
        static std::string url_encode(const std::string& input);


    private:
        static AlbumArtProvider* _instance; ///< The global AlbumArtProvider instance

    protected:
        AlbumArtProvider() {};
        virtual ~AlbumArtProvider() {};
};

#endif // ALBUM_ART_PROVIDER_H
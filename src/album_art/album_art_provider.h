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

/// @brief Abstracts web services that provide album art images.
class AlbumArtProvider {
    public:

        /// @brief Thrown when the provider cannot be reached.
        struct InitializationFailed : public std::exception {
            const char* what() const throw() {
                return "Album art provider could not initialize. Please check your environment variables.";
            }
        };

        /// @brief Thrown when the provider cannot find album art images.
        struct NotFoundException : public std::exception {
            const char* what() const throw() {
                return "Resource could not be found.";
            }
        };

        /// @brief An album art image and its URL.
        struct AlbumArt {
            Glib::RefPtr<Gdk::Pixbuf> art;
            std::string url;
        };

        /// @brief Factory method to return the album art provider.
        /// It may be called one or more times.
        /// Be sure to destroy() the instance in order to avoid leaks.
        /// It is suggested to do so in the teardown of the first caller.
        /// @return The provider that has been selected via configuration file.
        static AlbumArtProvider* instance();

        /// @brief Destroys the AlbumArtProvider.
        static void destroy();

        /// @brief Initializes the AlbumArtProvider
        /// (i.e. retrieve an API key).
        virtual void init();

        /// @brief Return all album art matching the criteria, at a specific width and height.
        /// @param [in] artist Album artist.
        /// @param [in] title  Album title.
        /// @param [in] width  Album art image width.
        /// @param [in] height Album art image height.
        /// @return A list of album art images.
        virtual std::vector<AlbumArt> album_art(const std::string& artist, const std::string& title, const int width, const int height) = 0;

        /// @brief             Retrieves the album art image at a given URL.
        /// @param [in] url    URL of the album art image to retrieve.
        /// @param [in] width  Width of the returned image.
        /// @param [in] height Height of the returned image.
        /// @return            An album art image.
        AlbumArt album_art(const std::string& url, const int width, const int height);

        /// @brief Returns the URL of the API endpoint with the given parameters encoded as query parameters.
        /// @param [in] url An API endpoint. This usually corresponds to an API method.
        /// @param [in] params The method parameters as key-value pairs.
        /// @return An API endpoint URL.
        static std::string url_with_params(const std::string& url, const std::map<std::string, std::string>& params);

        /// @brief Encodes an arbitrary string into a query parameter value.
        /// @param [in] input An arbitrary string.
        /// @return The URL encoded input.
        static std::string url_encode(const std::string& input);


    private:
        static AlbumArtProvider* _instance; ///< The global AlbumArtProvider instance

    protected:
        /// @brief AlbumArtProvider constructor.
        AlbumArtProvider() {};

        /// @brief AlbumArtProvider destructor.
        virtual ~AlbumArtProvider() {};
};

#endif // ALBUM_ART_PROVIDER_H
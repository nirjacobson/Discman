#ifndef LAST_FM_H
#define LAST_FM_H

#include <algorithm>
#include <sstream>
#include <string>
#include <map>
#include <exception>
#include <glibmm-2.4/glibmm.h>
#include <giomm/memoryinputstream.h>
#include <gdkmm/pixbuf.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>

class LastFM {

  public:

    struct NotFoundException : public std::exception {
        const char* what() const throw() {
            return "Resource could not be found.";
        }
    };

    enum Method {
      ALBUM_GET_INFO
    };
    static constexpr const char* BASE_URL = "http://ws.audioscrobbler.com/2.0/";
    static constexpr const char* API_KEY = "c4f9a47a4ca890c0981d1707ff28c434";

    static Glib::RefPtr<Gdk::Pixbuf> album_art(const std::string& artist, const std::string& title, const int width, const int height);

  private:
    static std::string method_name(const Method method);
    static std::string url(const Method method, const std::string& apiKey, const std::map<std::string, std::string>& params);
    static std::string url_with_params(const std::string& url, const std::map<std::string, std::string>& params);
};

#endif // LAST_FM_H
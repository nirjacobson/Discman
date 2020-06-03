#include "last_fm.h"

Glib::RefPtr<Gdk::Pixbuf> LastFM::album_art(const std::string& artist, const std::string& title, const int width, const int height) {
  cURLpp::Cleanup cleanup;
  cURLpp::Easy easyhandle;

  std::string request_url = url(Method::ALBUM_GET_INFO, API_KEY, {
    {"artist", artist},
    {"album", title}
  });

  std::stringstream ss;
  easyhandle.setOpt(cURLpp::Options::Url(request_url));
  easyhandle.setOpt(cURLpp::Options::WriteStream(&ss));
  easyhandle.perform();

  Json::Value root;
  ss >> root;
  std::cout << root << std::endl;
  Json::Value image = root["album"]["image"];

  for (unsigned int i = 0; i < image.size(); i++) {
    Json::Value element = image[i];
    if (element["size"] == "mega") {
      request_url = element["#text"].asString();
      if (!request_url.empty()) {
        easyhandle.setOpt(cURLpp::Options::Url(request_url));

        ss.str("");
        easyhandle.perform();

        std::string data = ss.str();
        Glib::RefPtr<Glib::Bytes> bytes = Glib::Bytes::create(data.c_str(), data.size());
        Glib::RefPtr<Gio::MemoryInputStream> is = Gio::MemoryInputStream::create();
        is->add_bytes(bytes);
        return Gdk::Pixbuf::create_from_stream_at_scale(is, width, height, true);
      }
    }
  }

  throw NotFoundException();
}

std::string LastFM::method_name(const Method method) {
  switch (method) {
    default:
    case ALBUM_GET_INFO:
      return "album.getinfo";
  }
}

std::string LastFM::url(const Method method, const std::string& apiKey, const std::map<std::string, std::string>& params) {
  std::map<std::string, std::string> full_params(params);
  full_params["method"] = method_name(method);
  full_params["api_key"] = apiKey;
  full_params["format"] = "json";

  return url_with_params(BASE_URL, full_params);
}

std::string LastFM::url_with_params(const std::string& url, const std::map<std::string, std::string>& params) {
  std::string new_url(url);
  if (new_url[new_url.length() - 1] != '/') new_url += '/';

  std::stringstream ss;
  ss << new_url;
  for (std::map<std::string, std::string>::const_iterator it=params.cbegin(); it!=params.cend(); ++it) {
    ss << (it == params.cbegin() ? '?' : '&');

    std::string formatted_value(it->second);
    std::replace(formatted_value.begin(), formatted_value.end(), ' ', '+');

    ss << it->first << '=' << formatted_value;
  }

  return ss.str();
}

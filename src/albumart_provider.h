#ifndef ALBUMART_PROVIDER_H
#define ALBUMART_PROVIDER_H

#include <glibmm.h>
#include <giomm/memoryinputstream.h>
#include <gdkmm/pixbuf.h>

class AlbumArtProvider {
    public:
        struct NotFoundException : public std::exception {
            const char* what() const throw() {
                return "Resource could not be found.";
            }
        };

        struct AlbumArt {
            Glib::RefPtr<Gdk::Pixbuf> art;
            std::string url;
        };

        static AlbumArtProvider* instance();

        virtual ~AlbumArtProvider() = 0;


        virtual void init();
        virtual std::vector<AlbumArt> album_art(const std::string& artist, const std::string& title, const int width, const int height) = 0;
        virtual AlbumArt album_art(const std::string& url, const int width, const int height) = 0;
    
    private:
        static AlbumArtProvider* _instance;
};

#endif // ALBUMART_PROVIDER_H
#ifndef ALBUMART_COMPONENT_H
#define ALBUMART_COMPONENT_H

#include <string>
#include <cmath>

#include <glibmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/viewport.h>
#include <gtkmm/image.h>
#include <sigc++/signal.h>

#include "album_art_provider.h"

class AlbumArtComponent {

    public:
        typedef sigc::signal<void()> sig_done;
        typedef sigc::signal<void(const std::string)> sig_art;

        static constexpr int ART_SIZE = 128;

        AlbumArtComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~AlbumArtComponent();

        sig_done signal_done();
        sig_art signal_art();

        void set_album_arts(const std::vector<AlbumArtProvider::AlbumArt>& arts, const int window_width);

    private:
        static constexpr int SPACING = 16;
        
        sig_done _signal_done;
        sig_art _signal_art;

        Gtk::Button* _done_button;
        Gtk::Viewport* _viewport;
        Gtk::Grid* _grid;

        void on_done_button_clicked();
};


#endif // ALBUMART_COMPONENT_H
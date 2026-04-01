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

#include "last_fm.h"

class AlbumArtComponent {

    public:
        typedef sigc::signal<void()> sig_done;
        typedef sigc::signal<void(const std::string)> sig_art;

        static constexpr int ART_SIZE = 128;

        AlbumArtComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~AlbumArtComponent();

        sig_done signal_done();
        sig_art signal_art();

        void set_albumarts(const std::vector<LastFM::AlbumArt>& arts, const int windowWidth);

    private:
        static constexpr int SPACING = 16;
        
        sig_done _signal_done;
        sig_art _signal_art;

        Gtk::Button* _doneButton;
        Gtk::Viewport* _viewport;
        Gtk::Grid* _grid;

        void on_done_button_clicked();
};


#endif // ALBUMART_COMPONENT_H
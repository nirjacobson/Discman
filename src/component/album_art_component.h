/**
 * @file album_art_component.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

#ifndef ALBUM_ART_COMPONENT
#define ALBUM_ART_COMPONENT

#include <string>
#include <cmath>

#include <glibmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/viewport.h>
#include <gtkmm/image.h>
#include <sigc++/signal.h>

#include "album_art/album_art_provider.h"
#include "component.h"

/// @brief Handles the interactions with the album art selection screen.
class AlbumArtComponent : public Component {

    public:
        
        /// @brief "Done with screen" signal type.
        typedef sigc::signal<void()> sig_done;
        
        /// @brief "Art selected" signal type.
        typedef sigc::signal<void(const std::string)> sig_art;

        /// @brief The width and height of art displayed on the album art selection screen.
        static constexpr int ART_SIZE = 128;

        /// @brief AlbumArtComponent constructor.
        /// @param [in] builder The builder initialized with the widgets on the album art selection screen.
        AlbumArtComponent(Glib::RefPtr<Gtk::Builder> builder);

        /// @brief AlbumArtComponent destructior.
        ~AlbumArtComponent();

        sig_done signal_done(); ///< Getter for ::_signal_done.
        sig_art signal_art();   ///< Getter for ::_signal_art.

        /// @brief Set the album art images to show on the selection screen.
        /// @param [in] arts Album art images.
        /// @param [in] window_width The current width of the application window.
        /// This is used to compute how many image columns to display.
        void set_album_arts(const std::vector<AlbumArtProvider::AlbumArt>& arts, const int window_width);

    private:
        /// @brief The spacing between images.
        static constexpr int SPACING = 16;

        sig_done _signal_done;     ///< Emitted when the user leaves the art selection screen.
        sig_art _signal_art;       ///< Emitted when the user selects an album art image.

        Gtk::Button* _done_button; ///< Done button on the art selection screen.
        Gtk::Viewport* _viewport;  ///< Viewport that displays the grid of album art.
        Gtk::Grid* _grid;          ///< Grid of album art.

        /// @brief Called when the "done" button is clicked.
        void on_done_button_clicked();
};


#endif // ALBUM_ART_COMPONENT
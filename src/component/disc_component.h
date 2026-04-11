/**
 * @file disc_component.h
 * @author Nir Jacobson
 * @date 2026-04-08
 */

#ifndef DISC_COMPONENT_H
#define DISC_COMPONENT_H

#include <glibmm.h>
#include <pangomm/layout.h>
#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/builder.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/liststore.h>
#include <gtkmm/cellrendererprogress.h>
#include <gtkmm/stack.h>
#include <gtkmm/window.h>
#include <sigc++/signal.h>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <discdb/disc.h>

#include "drive_manager/drive_manager.h"
#include "component.h"

/// @brief Handles the interactions with the album labels, track listing and ipod/eject buttons.
class DiscComponent : public Component {
    public:
        /// @brief "Drive eject requested" signal type.
        typedef sigc::signal<void(DriveManager::Drive)> sig_eject_requested;

        /// @brief "Track selected" signal type.
        typedef sigc::signal<void(unsigned int)> sig_track_selected;

        /// @brief "Rip requested" signal type.
        typedef sigc::signal<void(unsigned int)> sig_rip_requested;

        /// @brief DiscComponent constructor.
        /// @param [in] drive_manager Used to eject the disc .
        /// @param [in] builder The builder initialized with the widgets managed by this component.
        DiscComponent(DriveManager& drive_manager, Glib::RefPtr<Gtk::Builder> builder);

        /// @brief DiscComponent destructor.
        ~DiscComponent();

        /// @brief Sets the DiscDB::Disc containing all information about the inserted disc.
        /// @param [in] disc The album information.
        void set_disc(const DiscDB::Disc* const disc);

        /// @brief Clears the selection in the track listing.
        void clear_selection();

        /// @brief Sets the selection in the track listing.
        /// @param [in] track The 1-based index of the track to select.
        void set_selection(unsigned int track);

        /// @brief Shows or hides the rip progress column in the track listing.
        /// @param [in] show Show (true) or hide (false).
        void show_progress(const bool show);

        /// @brief Updates the percentage completion in the track listing
        /// for a given track with a given percentage.
        /// @param [in] track    1-based index of the track to update.
        /// @param [in] progress New percentage completion.
        void update_track_progress(unsigned int track, unsigned int progress);

        /// @brief Shows or hides the iPod button (rip button).
        /// @param [in] show Show (true) or hide (false).
        void show_ipod_button(const bool show);

        /// @brief Enables or disables the iPod button (rip button).
        /// @param [in] enable Wnable (true) or disable (false).
        void enable_ipod_button(const bool enable);

        /// @brief Enables or disables the normal eject button.
        /// @param [in] enable Wnable (true) or disable (false).
        void enable_eject_button(const bool enable);

        /// @brief Switches between the normal eject button
        /// and the eject button with disc/iPod selection.
        /// @param [in] show Show (true) or hide (false).
        void show_double_eject_button(const bool show);

        sig_eject_requested signal_eject_requested();   ///< Getter for ::_signal_eject_requested.
        sig_rip_requested signal_rip_requested();       ///< Getter for ::_signal_rip_requested.
        sig_track_selected signal_track_selected();     ///< Getter for ::_signal_track_selected.

    private:

        /// @brief Track list columns.
        class TracksListColumnRecord : public Gtk::TreeModel::ColumnRecord {
            public:
                TracksListColumnRecord() {
                    this->add(this->number_column);
                    this->add(this->title_column);
                    this->add(this->progress_column);
                    this->add(this->length_column);
                }

                Gtk::TreeModelColumn<guint> number_column;         ///< Track number column.
                Gtk::TreeModelColumn<Glib::ustring> title_column;  ///< Track title column.
                Gtk::TreeModelColumn<int> progress_column;         ///< Track rip progress column.
                Gtk::TreeModelColumn<Glib::ustring> length_column; ///< Track length column.
        };

        /// @brief Used to eject the disc.
        DriveManager& _drive_manager;

        Gtk::Label* _album_label;                                 ///< Label displaying the album title.
        Gtk::Label* _album_artist_label;                          ///< Label displaying the album artist.

        Gtk::Button* _eject_button;                               ///< Normal eject button.
        Gtk::Stack* _eject_button_stack;                          ///< Stack that switches between eject buttons.
        Gtk::MenuButton* _double_eject_button;                    ///< Eject button with disc/iPod selection.
        Glib::RefPtr<Gio::Menu> _double_eject_button_menu;        ///< Disc/iPod eject button selection menu.
        Glib::RefPtr<Gio::SimpleActionGroup> _eject_action_group; ///< Action group for disc/iPod eject button menu.

        Gtk::Button* _ipod_button;                                ///< Shown when the iPod is connected, enabled when ripping is possible.
        Gtk::TreeView* _tracks_tree_view;                         ///< Shows the track list.
        Glib::RefPtr<Gtk::ListStore> _tracks_list_store;          ///< Track list model.

        sig_eject_requested _signal_eject_requested;              ///< Emitted when an eject button is used.
        sig_rip_requested _signal_rip_requested;                  ///< Emitted when the iPod (rip) button is clicked.
        sig_track_selected _signal_track_selected;                ///< Emitted when a track is double-clicked in the track list.

        /// @brief Called after successful ejection of a drive.
        /// @param [in] drive The drive that was ejected (disc or iPod) .
        void on_eject(const DriveManager::Drive drive);
        void on_eject_button_clicked();                           ///< Called when the normal eject button is clicked.
        void on_ipod_button_clicked();                            ///< Called when the iPod (rip) button is clicked.

        /// @brief Called when a track is double-clicked in the track list.
        /// @param [in] path Tree path to the track row double-clicked.
        void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*);
};

#endif // DISC_COMPONENT_H

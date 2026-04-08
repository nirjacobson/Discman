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

#include "drive_manager.h"

/// @brief Handles the interactions with the album labels, track listing and ipod/eject buttons
class DiscComponent {
    public:
        typedef sigc::signal<void(DriveManager::Drive)> sig_eject_requested;
        typedef sigc::signal<void(unsigned int)> sig_track_selected;
        typedef sigc::signal<void(unsigned int)> sig_rip_requested;

        /// @brief DiscComponent constructor
        /// @param [in] drive_manager Used to eject the disc 
        /// @param [in] builder The builder initialized with the widgets managed by this component
        DiscComponent(DriveManager& drive_manager, Glib::RefPtr<Gtk::Builder> builder);
        ~DiscComponent();

        /// @brief Sets the DiscDB::Disc containing all information about the inserted disc
        /// @param [in] disc the album information
        void set_disc(const DiscDB::Disc* const disc);

        /// @brief Clears the selection in the track listing
        void clear_selection();

        /// @brief Sets the selection in the track listing
        /// @param [in] track the 1-based index of the track to select
        void set_selection(unsigned int track);

        /// @brief Shows or hides the rip progress column in the track listing
        /// @param [in] show show (true) or hide (false)
        void show_progress(const bool show);

        /// @brief Updates the percentage completion in the track listing
        /// for a given track with a given percentage
        /// @param [in] track    the 1-based index of the track to update 
        /// @param [in] progress the new percentage completion
        void update_track_progress(unsigned int track, unsigned int progress);

        /// @brief Shows or hides the iPod button (rip button)
        /// @param [in] show show (true) or hide (false)
        void show_ipod_button(const bool show);

        /// @brief Enables or disables the iPod button (rip button)
        /// @param [in] enable enable (true) or disable (false)
        void enable_ipod_button(const bool enable);

        /// @brief Enables or disables the normal eject button
        /// @param [in] enable enable (true) or disable (false)
        void enable_eject_button(const bool enable);

        /// @brief Switches between the normal eject button
        /// and the eject button with disc/iPod selection
        /// @param [in] show show (true) or hide (false)
        void show_double_eject_button(const bool show);

        sig_eject_requested signal_eject_requested();
        sig_rip_requested signal_rip_requested();
        sig_track_selected signal_track_selected();

    private:

        /// @brief Track list columns
        class TracksListColumnRecord : public Gtk::TreeModel::ColumnRecord {
            public:
                TracksListColumnRecord() {
                    this->add(this->number_column);
                    this->add(this->title_column);
                    this->add(this->progress_column);
                    this->add(this->length_column);
                }

                Gtk::TreeModelColumn<guint> number_column;         ///< Track number column
                Gtk::TreeModelColumn<Glib::ustring> title_column;  ///< Track title column
                Gtk::TreeModelColumn<int> progress_column;         ///< Track rip progress column
                Gtk::TreeModelColumn<Glib::ustring> length_column; ///< Track length column
        };

        /// @brief Used to eject the disc
        DriveManager& _drive_manager;

        Gtk::Label* _album_label;        ///< The label displaying the album title
        Gtk::Label* _album_artist_label; ///< The label displaying the album artist

        Gtk::Button* _eject_button;                               ///< The normal eject button
        Gtk::Stack* _eject_button_stack;                          ///< The stack that switches between eject buttons
        Gtk::MenuButton* _double_eject_button;                    ///< The eject button with disc/iPod selection
        Glib::RefPtr<Gio::Menu> _double_eject_button_menu;        ///< The disc/iPod eject button selection menu
        Glib::RefPtr<Gio::SimpleActionGroup> _eject_action_group; ///< Action group for disc/iPod eject button menu

        Gtk::Button* _ipod_button;                                ///< Shown when the iPod is connected, enabled when ripping is possible
        Gtk::TreeView* _tracks_tree_view;                         ///< Shows the track list
        Glib::RefPtr<Gtk::ListStore> _tracks_list_store;          ///< The track list model

        sig_eject_requested _signal_eject_requested;              ///< Emitted when an eject button is used
        sig_rip_requested _signal_rip_requested;                  ///< Emitted when the iPod (rip) button is clicked
        sig_track_selected _signal_track_selected;                ///< Emitted when a track is double-clicked in the track list

        /// @brief Called after successful ejection of a drive
        /// @param [in] drive the drive that was ejected (disc or iPod) 
        void on_eject(const DriveManager::Drive drive);
        void on_eject_button_clicked();                           ///< Called when the normal eject button is clicked
        void on_ipod_button_clicked();                            ///< Called when the iPod (rip) button is clicked

        /// @brief Called when a track is double-clicked in the track list
        /// @param [in] path The tree path to the track row double-clicked
        void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*);
};

#endif // DISC_COMPONENT_H

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

class DiscComponent {
    public:
        typedef sigc::signal<void(DriveManager::Drive)> sig_eject_requested;
        typedef sigc::signal<void(unsigned int)> sig_track_selected;
        typedef sigc::signal<void(unsigned int)> sig_rip_requested;

        DiscComponent(DriveManager& drive_manager, Glib::RefPtr<Gtk::Builder> builder);
        ~DiscComponent();

        void set_disc(const DiscDB::Disc* const disc);
        void clear_selection();
        void set_selection(unsigned int track);

        void show_progress(const bool show);

        void update_track_progress(unsigned int track, unsigned int progress);

        void show_ipod_button(const bool show);
        void enable_ipod_button(const bool enable);
        void enable_eject_button(const bool enable);
        void show_double_eject_button(const bool show);

        sig_eject_requested signal_eject_requested();
        sig_rip_requested signal_rip_requested();
        sig_track_selected signal_track_selected();

    private:

        class TracksListColumnRecord : public Gtk::TreeModel::ColumnRecord {
            public:
                TracksListColumnRecord() {
                    this->add(this->number_column);
                    this->add(this->title_column);
                    this->add(this->progress_column);
                    this->add(this->length_column);
                }

                Gtk::TreeModelColumn<guint> number_column;
                Gtk::TreeModelColumn<Glib::ustring> title_column;
                Gtk::TreeModelColumn<int> progress_column;
                Gtk::TreeModelColumn<Glib::ustring> length_column;
        };

        DriveManager& _drive_manager;

        Gtk::Label* _album_label;
        Gtk::Label* _album_artist_label;

        Gtk::Button* _eject_button;
        Gtk::Stack* _eject_button_stack;
        Gtk::MenuButton* _double_eject_button;
        Glib::RefPtr<Gio::Menu> _double_eject_button_menu;
        Glib::RefPtr<Gio::SimpleActionGroup> _eject_action_group;

        Gtk::Button* _ipod_button;
        Gtk::TreeView* _tracks_tree_view;
        Glib::RefPtr<Gtk::ListStore> _tracks_list_store;

        sig_eject_requested _signal_eject_requested;
        sig_rip_requested _signal_rip_requested;
        sig_track_selected _signal_track_selected;

        void on_eject(const DriveManager::Drive drive);
        void on_eject_button_clicked();
        void on_ipod_button_clicked();
        void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*);
};

#endif // DISC_COMPONENT_H

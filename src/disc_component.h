#ifndef DISC_COMPONENT_H
#define DISC_COMPONENT_H

#include <glibmm-2.4/glibmm.h>
#include <gtkmm-3.0/gtkmm/builder.h>
#include <gtkmm-3.0/gtkmm/button.h>
#include <gtkmm-3.0/gtkmm/label.h>
#include <gtkmm-3.0/gtkmm/treeview.h>
#include <gtkmm-3.0/gtkmm/treemodel.h>
#include <gtkmm-3.0/gtkmm/liststore.h>
#include <sigc++/signal.h>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "disc.h"
#include "last_fm.h"

class DiscComponent {
    public:
        typedef sigc::signal<void> sig_eject_requested;
        typedef sigc::signal<void, unsigned int> sig_track_selected;

        DiscComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~DiscComponent();

        void set_disc(const DiscDB::Disc* const disc);
        void clear_selection();
        void set_selection(unsigned int track);

        sig_eject_requested signal_eject_requested();
        sig_track_selected signal_track_selected();

    private:

        class TracksListColumnRecord : public Gtk::TreeModel::ColumnRecord {
            public:
                TracksListColumnRecord() {
                    this->add(this->numberColumn);
                    this->add(this->titleColumn);
                    this->add(this->lengthColumn);
                }

                Gtk::TreeModelColumn<guint> numberColumn;
                Gtk::TreeModelColumn<Glib::ustring> titleColumn;
                Gtk::TreeModelColumn<Glib::ustring> lengthColumn;
        };

        Gtk::Label* _albumLabel;
        Gtk::Label* _albumArtistLabel;
        Gtk::Button* _ejectButton;
        Gtk::TreeView* _tracksTreeView;
        Glib::RefPtr<Gtk::ListStore> _tracksListStore;

        sig_eject_requested _signal_eject_requested;
        sig_track_selected _signal_track_selected;

        void on_eject_button_clicked();
        void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*);
};

#endif // DISC_COMPONENT_H

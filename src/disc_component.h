#ifndef DISC_COMPONENT_H
#define DISC_COMPONENT_H

#include <glibmm.h>
#include <pangomm/layout.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/liststore.h>
#include <gtkmm/cellrendererprogress.h>
#include <sigc++/signal.h>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <discdb/disc.h>

class DiscComponent {
    public:
        typedef sigc::signal<void()> sig_eject_requested;
        typedef sigc::signal<void(unsigned int)> sig_track_selected;
        typedef sigc::signal<void(unsigned int)> sig_rip_requested;

        DiscComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~DiscComponent();

        void set_disc(const DiscDB::Disc* const disc);
        void clear_selection();
        void set_selection(unsigned int track);

        void show_progress();
        void rip_done();

        void update_track_progress(unsigned int track, unsigned int progress);

        sig_eject_requested signal_eject_requested();
        sig_rip_requested signal_rip_requested();
        sig_track_selected signal_track_selected();

    private:

        class TracksListColumnRecord : public Gtk::TreeModel::ColumnRecord {
            public:
                TracksListColumnRecord() {
                    this->add(this->numberColumn);
                    this->add(this->titleColumn);
                    this->add(this->progressColumn);
                    this->add(this->lengthColumn);
                }

                Gtk::TreeModelColumn<guint> numberColumn;
                Gtk::TreeModelColumn<Glib::ustring> titleColumn;
                Gtk::TreeModelColumn<int> progressColumn;
                Gtk::TreeModelColumn<Glib::ustring> lengthColumn;
        };

        Gtk::Label* _albumLabel;
        Gtk::Label* _albumArtistLabel;
        Gtk::Button* _ejectButton;
        Gtk::Button* _ipodButton;
        Gtk::TreeView* _tracksTreeView;
        Glib::RefPtr<Gtk::ListStore> _tracksListStore;

        sig_eject_requested _signal_eject_requested;
        sig_rip_requested _signal_rip_requested;
        sig_track_selected _signal_track_selected;

        void on_eject_button_clicked();
        void on_ipod_button_clicked();
        void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*);
};

#endif // DISC_COMPONENT_H

#ifndef BLUETOOTH_COMPONENT_H
#define BLUETOOTH_COMPONENT_H

#include <glibmm-2.4/glibmm.h>
#include <gtkmm-3.0/gtkmm/builder.h>
#include <gtkmm-3.0/gtkmm/button.h>
#include <gtkmm-3.0/gtkmm/label.h>
#include <gtkmm-3.0/gtkmm/treeview.h>
#include <gtkmm-3.0/gtkmm/liststore.h>
#include <sigc++/signal.h>

class BluetoothComponent {

  public:
    typedef sigc::signal<void> sig_done;

    BluetoothComponent(Glib::RefPtr<Gtk::Builder> builder);
    ~BluetoothComponent();

    sig_done signal_done();

  private:

    Gtk::Label* _deviceLabel;
    Gtk::Label* _deviceStatusLabel;
    Gtk::TreeView* _devicesTreeView;
    Glib::RefPtr<Gtk::ListStore> _devicesListStore;
    Gtk::Button* _doneButton;
    Gtk::Button* _connectButton;

    sig_done _signal_done;

    void on_done_button_clicked();

};

#endif // BLUETOOTH_COMPONENT_H
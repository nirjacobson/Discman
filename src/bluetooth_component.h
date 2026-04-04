#ifndef BLUETOOTH_COMPONENT_H
#define BLUETOOTH_COMPONENT_H

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <regex>

#include <glibmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/liststore.h>
#include <sigc++/signal.h>

#include <bluez/adapter.h>

class BluetoothComponent {

    public:
        typedef sigc::signal<void()> sig_done;
        typedef sigc::signal<void()> sig_conn;

        BluetoothComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~BluetoothComponent();

        void on_show();
        void on_hide();
        void on_device_initialization_complete(const bool initialized);

        sig_done signal_done();
        sig_conn signal_connected();

    private:

        enum DeviceInitialization {
            NotInitialized,
            Initialized,
            Error
        };

        class DevicesListColumnRecord : public Gtk::TreeModel::ColumnRecord {
            public:
                DevicesListColumnRecord() {
                    this->add(this->address_column);
                    this->add(this->name_column);
                }

                Gtk::TreeModelColumn<Glib::ustring> address_column;
                Gtk::TreeModelColumn<Glib::ustring> name_column;
        };

        sig_conn _signal_connected;

        Bluez::Adapter _adapter;
        std::string _alsa_device_address;
        Bluez::Device* _alsa_device;

        Gtk::Label* _device_label;
        Gtk::Label* _device_status_label;
        Gtk::TreeView* _devices_tree_view;
        Glib::RefPtr<Gtk::ListStore> _devices_list_store;
        Gtk::Button* _done_button;
        Gtk::Button* _connect_button;

        DeviceInitialization _alsa_device_init;

        sig_done _signal_done;

        void on_done_button_clicked();
        void on_device_added(const std::string& address);
        void on_device_removed(const std::string& address);
        void on_devices_list_selection_changed();
        void on_connect_button_clicked();

        void on_device_status_change();

        void build_devices_list();
        void get_alsa_device_address();
        void try_get_alsa_device();
        void set_device_labels();

        void update_asoundrc(const std::string& address);

};

#endif // BLUETOOTH_COMPONENT_H
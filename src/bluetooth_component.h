/**
 * @file bluetooth_component.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

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

/// @brief Handles the interactions with the Bluetooth device selection screen
class BluetoothComponent {

    public:
        typedef sigc::signal<void()> sig_done;
        typedef sigc::signal<void()> sig_conn;

        /// @brief BluetoothComponent constructor
        /// @param [in] builder The builder initialized with the widgets on the Bluetooth device selection screen
        BluetoothComponent(Glib::RefPtr<Gtk::Builder> builder);
        ~BluetoothComponent();

        void on_show(); ///< Called when the Bluetooth device selection screen is navigated to
        void on_hide(); ///< Called when the Bluetooth device selection screen is navigated from
        void on_device_initialization_complete(const bool initialized);

        sig_done signal_done();
        sig_conn signal_connected();

    private:

        /// @brief Bluetooth device initialization state
        enum DeviceInitialization {
            NotInitialized,
            Initialized,
            Error
        };

        /// @brief Bluetooth device list columns
        class DevicesListColumnRecord : public Gtk::TreeModel::ColumnRecord {
            public:
                DevicesListColumnRecord() {
                    this->add(this->address_column);
                    this->add(this->name_column);
                }

                Gtk::TreeModelColumn<Glib::ustring> address_column;
                Gtk::TreeModelColumn<Glib::ustring> name_column;
        };

        sig_conn _signal_connected;                         ///< Emitted when the chosen device has been connected to

        Bluez::Adapter _adapter;                            ///< The Bluetooth adapter used for pairing and connection
        std::string _alsa_device_address;                   ///< The MAC address of the chosen device
        Bluez::Device* _alsa_device;                        ///< The Bluetooth device retrieved from _adapter.

        Gtk::Label* _device_label;                          ///< Reference to the label displaying the chosen device alias
        Gtk::Label* _device_status_label;                   ///< Reference to the label displaying chosen device connection status
        Gtk::TreeView* _devices_tree_view;                  ///< Reference to the device list
        Glib::RefPtr<Gtk::ListStore> _devices_list_store;   ///< Reference to the device list model
        Gtk::Button* _done_button;                          ///< Reference to the Done button
        Gtk::Button* _connect_button;                       ///< Reference to the Connect/Disconnect button

        DeviceInitialization _alsa_device_init;             ///< The initialization status of the chosen device

        sig_done _signal_done;                              ///< Emitted when the Done button is clicked

        void on_done_button_clicked();                      ///< Called when the Done button is clicked
        void on_device_added(const std::string& address);   ///< Called when _adapter discovers a new device
        void on_device_removed(const std::string& address); ///< Called when _adapter reports a removed device
        void on_devices_list_selection_changed();           ///< Called when a device is clicked in the device list
        void on_connect_button_clicked();                   ///< Called when the Connect/Disconnect button is clicked

        void on_device_status_change();                     ///< Called when the chosen device pairs, connects or disconnects

        void build_devices_list();                          ///< Build the initial list of preregistered Bluetooth devices
        void get_alsa_device_address();                     ///< Retrieve the device MAC address from the ALSA configuration file
        void try_get_alsa_device();                         ///< Attempts to set _alsa_device based on _alsa_device_address.
        void set_device_labels();                           ///< Update _device_label and _device_status_label based on current state

        /// @brief Update the ALSA configuration file with the newly selected Bluetooth device
        /// @param [in] address the MAC address of the selected device
        void update_asoundrc(const std::string& address);

};

#endif // BLUETOOTH_COMPONENT_H
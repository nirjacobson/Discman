#include "bluetooth_component.h"

BluetoothComponent::BluetoothComponent(Glib::RefPtr<Gtk::Builder> builder)
    : _adapter("hci0")
    , _alsaDevice(nullptr)
    , _alsaDeviceInitialization(DeviceInitialization::NotInitialized) {
    builder->get_widget("deviceLabel", _deviceLabel);
    builder->get_widget("deviceStatusLabel", _deviceStatusLabel);
    builder->get_widget("devicesTreeView", _devicesTreeView);
    builder->get_widget("bluetoothDoneButton", _doneButton);
    builder->get_widget("connectButton", _connectButton);

    _devicesListStore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(
                            builder->get_object("devicesListStore")
                        );

    DevicesListColumnRecord cols;
    _devicesTreeView->append_column("Devices", cols.nameColumn);

    build_devices_list();

    _devicesTreeView->get_selection()->signal_changed().connect(sigc::mem_fun(this, &BluetoothComponent::on_devices_list_selection_changed));

    _connectButton->signal_clicked().connect(sigc::mem_fun(this, &BluetoothComponent::on_connect_button_clicked));

    _doneButton->signal_clicked().connect(sigc::mem_fun(this, &BluetoothComponent::on_done_button_clicked));

    _adapter.signal_device_added().connect(sigc::mem_fun(this, &BluetoothComponent::on_device_added));
    _adapter.signal_device_removed().connect(sigc::mem_fun(this, &BluetoothComponent::on_device_removed));
}

BluetoothComponent::~BluetoothComponent() {
    if (_alsaDevice)
        delete _alsaDevice;

    delete _connectButton;
    delete _doneButton;
    delete _devicesTreeView;
    delete _deviceStatusLabel;
    delete _deviceLabel;
}

void BluetoothComponent::on_show() {
    get_alsa_device_address();
    try_get_alsa_device();
    set_device_labels();
    _adapter.startDiscovery();
}

void BluetoothComponent::on_hide() {
    _adapter.stopDiscovery();
}

void BluetoothComponent::on_device_initialization_complete(const bool initialized) {
    _alsaDeviceInitialization = initialized ? DeviceInitialization::Initialized : DeviceInitialization::Error;
    set_device_labels();
}

BluetoothComponent::sig_done BluetoothComponent::signal_done() {
    return _signal_done;
}

BluetoothComponent::sig_conn BluetoothComponent::signal_connected() {
    return _signal_connected;
}

void BluetoothComponent::on_done_button_clicked() {
    _signal_done.emit();
}

void BluetoothComponent::on_device_added(const std::string& address) {
    DevicesListColumnRecord cols;

    auto row = *(_devicesListStore->append());
    row[cols.addressColumn] = address;
    row[cols.nameColumn] = _adapter.alias(address);

    _devicesTreeView->get_vadjustment()->set_value(_devicesTreeView->get_vadjustment()->get_upper());

    if (!_alsaDevice && _alsaDeviceAddress == address) {
        try_get_alsa_device();
        set_device_labels();
    }
}

void BluetoothComponent::on_device_removed(const std::string& address) {
    Gtk::TreeModel::iterator it = _devicesListStore->get_iter("0");

    DevicesListColumnRecord cols;

    for (; it; it++) {
        Gtk::TreeModel::Row row = *it;
        if (row[cols.addressColumn] == address) {
            _devicesListStore->erase(it);
            break;
        }
    }

    if (_alsaDevice && _alsaDeviceAddress == address) {
        delete _alsaDevice;
        _alsaDevice = nullptr;
        set_device_labels();
    }
}

void BluetoothComponent::on_devices_list_selection_changed() {
    std::string selectedAddress = _devicesTreeView->get_selection()->get_selected()->get_value(DevicesListColumnRecord().addressColumn);

    bool showDisconnect = (_alsaDevice && _alsaDevice->address() == selectedAddress && _alsaDevice->connected());

    _connectButton->set_sensitive(!_devicesTreeView->get_selection()->get_selected_rows().empty());
    _connectButton->set_label(showDisconnect ? "Disconnect" : "Connect");
}

void BluetoothComponent::update_asoundrc(const std::string& address) {
    std::ifstream asoundrc;
    std::ofstream asoundrc_tmp;
    std::string line;
    std::regex mac_pattern("([0-9A-F]{2}:){5}[0-9A-F]{2}");

    asoundrc.open("/home/pi/.asoundrc");
    asoundrc_tmp.open("/home/pi/.asoundrc.tmp");
    if (asoundrc.good()) {
        while (getline(asoundrc, line)) {
            std::string outLine = std::regex_replace(line, mac_pattern, address);
            asoundrc_tmp << outLine << std::endl;
        }
    }
    asoundrc_tmp.close();
    std::filesystem::rename("/home/pi/.asoundrc.tmp", "/home/pi/.asoundrc");
}

void BluetoothComponent::on_connect_button_clicked() {
    bool connect = _connectButton->get_label() == "Connect";
    std::string selectedAddress = _devicesTreeView->get_selection()->get_selected()->get_value(DevicesListColumnRecord().addressColumn);

    if (connect) {
        if (_alsaDevice)
            _alsaDevice->disconnect();
        _alsaDeviceInitialization = DeviceInitialization::NotInitialized;

        update_asoundrc(selectedAddress);
        get_alsa_device_address();
        try_get_alsa_device();
        set_device_labels();
        assert(_alsaDevice);
        _alsaDevice->connect();
    } else {
        assert(_alsaDevice);
        _alsaDevice->disconnect();
    }
}

void BluetoothComponent::on_device_status_change() {
    set_device_labels();
    if (_alsaDevice->connected()) {
        _signal_connected.emit();
    }
}

void BluetoothComponent::build_devices_list() {
    std::vector<std::string> devices = _adapter.devices();

    _devicesListStore->clear();

    DevicesListColumnRecord cols;

    for (unsigned int i = 0; i < devices.size(); i++) {
        auto row = *(_devicesListStore->append());
        row[cols.addressColumn] = devices[i];
        row[cols.nameColumn] = _adapter.alias(devices[i]);
    }
}

void BluetoothComponent::get_alsa_device_address() {
    std::ifstream asoundrc;
    std::string line;
    std::regex mac_pattern("([0-9A-F]{2}:){5}[0-9A-F]{2}");

    asoundrc.open("/home/pi/.asoundrc");
    if (asoundrc.good()) {
        while (getline(asoundrc, line)) {
            std::smatch match;

            if (std::regex_search(line, match, mac_pattern)) {
                _alsaDeviceAddress = match[0];
                return;
            }
        }
    }
}

void BluetoothComponent::try_get_alsa_device() {
    try {
        if (_alsaDevice)
            delete _alsaDevice;

        _alsaDevice = _adapter.device(_alsaDeviceAddress);
        _alsaDevice->signal_paired().connect(sigc::mem_fun(this, &BluetoothComponent::on_device_status_change));
        _alsaDevice->signal_connected().connect(sigc::mem_fun(this, &BluetoothComponent::on_device_status_change));
        _alsaDevice->signal_disconnected().connect(sigc::mem_fun(this, &BluetoothComponent::on_device_status_change));
    } catch (const Bluez::Adapter::DeviceNotFound& e) {
        _alsaDevice = nullptr;
    }
}

void BluetoothComponent::set_device_labels() {
    if (_alsaDevice) {
        _deviceLabel->set_text(_alsaDevice->alias());

        if (_alsaDevice->connected()) {
            if (_alsaDeviceInitialization == DeviceInitialization::Initialized) {
                _deviceStatusLabel->set_text("Ready.");
            } else if (_alsaDeviceInitialization == DeviceInitialization::Error) {
                _deviceStatusLabel->set_text("Initialization failed.");
            } else {
                _deviceStatusLabel->set_text("Connected.");
            }
        } else if (_alsaDevice->paired()) {
            _deviceStatusLabel->set_text("Paired.");
        } else {
            _deviceStatusLabel->set_text("Not connected.");
        }
    } else {
        _deviceLabel->set_text("Not Connected");
        _deviceStatusLabel->set_text("Please select a device.");
    }
}

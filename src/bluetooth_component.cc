#include "bluetooth_component.h"

BluetoothComponent::BluetoothComponent(Glib::RefPtr<Gtk::Builder> builder)
    : _adapter("hci0")
    , _alsa_device(nullptr)
    , _alsa_device_init(DeviceInitialization::NotInitialized) {
    _device_label = builder->get_widget<Gtk::Label>("deviceLabel");
    _device_status_label = builder->get_widget<Gtk::Label>("deviceStatusLabel");
    _devices_tree_view = builder->get_widget<Gtk::TreeView>("devicesTreeView");
    _done_button = builder->get_widget<Gtk::Button>("bluetoothDoneButton");
    _connect_button = builder->get_widget<Gtk::Button>("connectButton");

    _devices_list_store = std::dynamic_pointer_cast<Gtk::ListStore>(builder->get_object("devicesListStore"));

    DevicesListColumnRecord cols;
    _devices_tree_view->append_column("Devices", cols.name_column);

    _devices_tree_view->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &BluetoothComponent::on_devices_list_selection_changed));

    _connect_button->signal_clicked().connect(sigc::mem_fun(*this, &BluetoothComponent::on_connect_button_clicked));

    _done_button->signal_clicked().connect(sigc::mem_fun(*this, &BluetoothComponent::on_done_button_clicked));

    _adapter.signal_device_added().connect(sigc::mem_fun(*this, &BluetoothComponent::on_device_added));
    _adapter.signal_device_removed().connect(sigc::mem_fun(*this, &BluetoothComponent::on_device_removed));
}

BluetoothComponent::~BluetoothComponent() {
    if (_alsa_device)
        delete _alsa_device;

    delete _connect_button;
    delete _done_button;
    delete _devices_tree_view;
    delete _device_status_label;
    delete _device_label;
}

void BluetoothComponent::on_show() {
    build_devices_list();

    get_alsa_device_address();
    try_get_alsa_device();
    set_device_labels();

    _adapter.start_discovery();
}

void BluetoothComponent::on_hide() {
    _adapter.stop_discovery();
}

void BluetoothComponent::on_device_initialization_complete(const bool initialized) {
    _alsa_device_init = initialized ? DeviceInitialization::Initialized : DeviceInitialization::Error;
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

    auto row = *(_devices_list_store->append());
    row[cols.address_column] = address;
    row[cols.name_column] = _adapter.alias(address);

    _devices_tree_view->get_vadjustment()->set_value(_devices_tree_view->get_vadjustment()->get_upper());

    if (!_alsa_device && _alsa_device_address == address) {
        try_get_alsa_device();
        set_device_labels();
    }
}

void BluetoothComponent::on_device_removed(const std::string& address) {
    Gtk::TreeModel::iterator it = _devices_list_store->get_iter("0");

    DevicesListColumnRecord cols;

    for (; it; it++) {
        Gtk::TreeModel::Row row = *it;
        if (row[cols.address_column] == address.data()) {
            _devices_list_store->erase(it);
            break;
        }
    }

    if (_alsa_device && _alsa_device_address == address) {
        delete _alsa_device;
        _alsa_device = nullptr;
        set_device_labels();
    }
}

void BluetoothComponent::on_devices_list_selection_changed() {
    std::string selectedAddress = _devices_tree_view->get_selection()->get_selected()->get_value(DevicesListColumnRecord().address_column);

    const bool showDisconnect = (_alsa_device && _alsa_device->address() == selectedAddress && _alsa_device->connected());

    _connect_button->set_sensitive(!_devices_tree_view->get_selection()->get_selected_rows().empty());
    _connect_button->set_label(showDisconnect ? "Disconnect" : "Connect");
}

void BluetoothComponent::update_asoundrc(const std::string& address) {
    std::ifstream asoundrc;
    std::ofstream asoundrc_tmp;
    std::string line;
    const std::regex mac_pattern("([0-9A-F]{2}:){5}[0-9A-F]{2}");

    asoundrc.open("/home/discman/.asoundrc");
    asoundrc_tmp.open("/home/discman/.asoundrc.tmp");
    if (asoundrc.good()) {
        while (getline(asoundrc, line)) {
            std::string outLine = std::regex_replace(line, mac_pattern, address);
            asoundrc_tmp << outLine << std::endl;
        }
    }
    asoundrc_tmp.close();
    std::filesystem::rename("/home/discman/.asoundrc.tmp", "/home/discman/.asoundrc");
}

void BluetoothComponent::on_connect_button_clicked() {
    const bool connect = _connect_button->get_label() == "Connect";
    const std::string selectedAddress = _devices_tree_view->get_selection()->get_selected()->get_value(DevicesListColumnRecord().address_column);

    if (connect) {
        if (_alsa_device)
            _alsa_device->disconnect();
        _alsa_device_init = DeviceInitialization::NotInitialized;

        update_asoundrc(selectedAddress);
        get_alsa_device_address();
        try_get_alsa_device();
        set_device_labels();
        assert(_alsa_device);
        _alsa_device->connect();
    } else {
        assert(_alsa_device);
        _alsa_device->disconnect();
    }
}

void BluetoothComponent::on_device_status_change() {
    set_device_labels();

    const std::string selectedAddress = _devices_tree_view->get_selection()->get_selected()->get_value(DevicesListColumnRecord().address_column);
    const bool showDisconnect = (_alsa_device && _alsa_device->address() == selectedAddress && _alsa_device->connected());
    _connect_button->set_label(showDisconnect ? "Disconnect" : "Connect");

    if (_alsa_device->connected()) {
        _signal_connected.emit();
    }
}

void BluetoothComponent::build_devices_list() {
    const std::vector<std::string> devices = _adapter.devices();

    _devices_list_store->clear();

    DevicesListColumnRecord cols;

    for (unsigned int i = 0; i < devices.size(); i++) {
        auto row = *(_devices_list_store->append());
        row[cols.address_column] = devices[i];
        row[cols.name_column] = _adapter.alias(devices[i]);
    }
}

void BluetoothComponent::get_alsa_device_address() {
    std::ifstream asoundrc;
    std::string line;
    const std::regex mac_pattern("([0-9A-F]{2}:){5}[0-9A-F]{2}");

    asoundrc.open("/home/discman/.asoundrc");
    if (asoundrc.good()) {
        while (getline(asoundrc, line)) {
            std::smatch match;

            if (std::regex_search(line, match, mac_pattern)) {
                _alsa_device_address = match[0];
                return;
            }
        }
    }
}

void BluetoothComponent::try_get_alsa_device() {
    try {
        if (_alsa_device)
            delete _alsa_device;

        _alsa_device = _adapter.device(_alsa_device_address);
        _alsa_device->signal_paired().connect(sigc::mem_fun(*this, &BluetoothComponent::on_device_status_change));
        _alsa_device->signal_connected().connect(sigc::mem_fun(*this, &BluetoothComponent::on_device_status_change));
        _alsa_device->signal_disconnected().connect(sigc::mem_fun(*this, &BluetoothComponent::on_device_status_change));
    } catch (const Bluez::Adapter::DeviceNotFound& e) {
        _alsa_device = nullptr;
    }
}

void BluetoothComponent::set_device_labels() {
    if (_alsa_device) {
        _device_label->set_text(_alsa_device->alias());

        if (_alsa_device->connected()) {
            if (_alsa_device_init == DeviceInitialization::Initialized) {
                _device_status_label->set_text("Ready.");
            } else if (_alsa_device_init == DeviceInitialization::Error) {
                _device_status_label->set_text("Initialization failed.");
            } else {
                _device_status_label->set_text("Connected.");
            }
        } else if (_alsa_device->paired()) {
            _device_status_label->set_text("Paired.");
        } else {
            _device_status_label->set_text("Not connected.");
        }
    } else {
        _device_label->set_text("Not Connected");
        _device_status_label->set_text("Please select a device.");
    }
}

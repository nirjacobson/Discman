#include "bluetooth_component.h"

BluetoothComponent::BluetoothComponent(Glib::RefPtr<Gtk::Builder> builder) {
  builder->get_widget("deviceLabel", _deviceLabel);
  builder->get_widget("deviceStatusLabel", _deviceStatusLabel);
  builder->get_widget("devicesTreeView", _devicesTreeView);
  builder->get_widget("bluetoothDoneButton", _doneButton);
  builder->get_widget("connectButton", _connectButton);

  _devicesListStore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(
    builder->get_object("devicesListStore")
  );

  _doneButton->signal_clicked().connect(sigc::mem_fun(this, &BluetoothComponent::on_done_button_clicked));
}

BluetoothComponent::~BluetoothComponent() {
  delete _connectButton;
  delete _doneButton;
  delete _devicesTreeView;
  delete _deviceStatusLabel;
  delete _deviceLabel;
}

BluetoothComponent::sig_done BluetoothComponent::signal_done() {
  return _signal_done;
}

void BluetoothComponent::on_done_button_clicked() {
  _signal_done.emit();
}
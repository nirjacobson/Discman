# cdplayer
A CD player built from the [Raspberry Pi 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/), [Pi Display](https://shop.pimoroni.com/products/raspberry-pi-7-touchscreen-display-with-frame) and [Apple SuperDrive](https://www.apple.com/shop/product/MD564LL/A/apple-usb-superdrive).

![cdplayer](https://nirjacobson.com/wp-content/uploads/2020/08/20200815_145216.jpg)

Once assembled...

## Install dependencies
```
emerge -av libcdio-paranoia portaudio gtkmm glibmm curlpp jsoncpp sg3_utils bluez-alsa
```

## Clone the code
```
git clone https://github.com/nirjacobson/libbluez.git
git clone https://github.com/nirjacobson/libdiscdb.git
git clone https://github.com/nirjacobson/cdplayer.git
```

## Build the libraries
```
cd libbluez
make
make install

cd libdiscdb
make
make install
```

## Build cdplayer
```
cd cdplayer
make
make install
```

## Configure ALSA for Bluetooth audio
`~/.asoundrc`:
```
defaults.bluealsa.service "org.bluealsa"
defaults.bluealsa.device "00:00:00:00:00:00"
defaults.bluealsa.profile "a2dp"
defaults.bluealsa.delay 10000

pcm.!default {
    type plug
    slave.pcm "bluealsa"
}
```

## Allow powering off as user

`/etc/polkit-1/rules.d/10-poweroff.rules`:
```
polkit.addRule(function(action, subject) {
    if (action.id == "org.freedesktop.login1.power-off" &&
        subject.user == "nir") {
        return polkit.Result.YES;
    }
});
```

## Enable the Apple SuperDrive at boot
`/etc/udev/rules.d/99-local.rules`:
```
# Initialise Apple SuperDrive
ACTION=="add", ATTRS{idProduct}=="1500", ATTRS{idVendor}=="05ac", DRIVERS=="usb", RUN+="/usr/bin/sg_raw /dev/$kernel EA 00 00 00 00 00 01"
```

## Set last.fm API key
`~/.bash_profile`:
```
...
export LAST_FM_API_KEY="YOUR_KEY_HERE"
```

## Run cdplayer at boot
TODO
## Credits

 [Buuf icon theme](https://www.gnome-look.org/p/1012512/)

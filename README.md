# Discman
A CD player built from the [Raspberry Pi 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/), [Pi Display](https://shop.pimoroni.com/products/raspberry-pi-7-touchscreen-display-with-frame) and [Apple SuperDrive](https://www.apple.com/shop/product/MD564LL/A/apple-usb-superdrive).

Once assembled...

## Install dependencies
```
emerge -av libcdio-paranoia portaudio gtkmm glibmm curlpp jsoncpp sg3_utils bluez-alsa stb udisks udiskie weston
```

## Clone the code
```
git clone https://github.com/nirjacobson/libdiscdb.git
git clone https://github.com/nirjacobson/libbluez.git
git clone https://github.com/nirjacobson/libudisks2.git
git clone https://github.com/nirjacobson/Discman.git
```

## Build the libraries
```
cd libdiscdb
make
make install

cd libbluez
make
make install

cd libudisks2
make
make install

```

## Build Discman
```
cd Discman
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
        subject.user == "<your username>") {
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

## Run Discman at boot
```
# systemctl edit getty@tty1.service

[Service]
Type=simple
ExecStart=
ExecStart=-/sbin/agetty -n --autologin discman --noclear %I 38400 linux
```

`~/.bash_profile`:
```
...

if tty | grep -q 'tty1'; then
    export DISCMAN_ALBUM_ART_PROVIDER="spotify" // Can be "lastfm" instead
    export SPOTIFY_CLIENT_ID=<your ID>          // Spotify client ID
    export SPOTIFY_CLIENT_SECRET=<your secret>  // Spotify client secret
    export LASTFM_API_kEY=<your key>            // last.fm API key (if using last.fm)

    weston --shell=kiosk-shell.so &
    export WAYLAND_DISPLAY=wayland-1
    sleep 8
    discman&
fi
```
## Credits

 [Buuf icon theme](https://www.gnome-look.org/p/1012512/)

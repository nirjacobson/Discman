# cdplayer
A CD player built from the [Raspberry Pi 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/), [Pi Display](https://shop.pimoroni.com/products/raspberry-pi-7-touchscreen-display-with-frame) and [Apple SuperDrive](https://www.apple.com/shop/product/MD564LL/A/apple-usb-superdrive).

![cdplayer](https://nirjacobson.com/wp-content/uploads/2020/08/20200815_145216.jpg)

Once assembled...

## Install dependencies
### Raspbian:
```
sudo apt install libcdio-paranoia-dev portaudio19-dev libgtkmm-3.0-dev libglibmm-2.4-dev libjsoncpp-dev libcurlpp-dev libcurl4-openssl-dev sg3-utils bluealsa xorg
```
### Gentoo:
```
emerge -av sudo libcdio-paranoia portaudio gtkmm glibmm jsoncpp sg3_utils bluez-alsa
```
Manually build:
```
https://github.com/jpbarrette/curlpp
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

## (Raspbian) Add user to the bluetooth group
```
usermod -a -G bluetooth pi
```

## (Gentoo)
`/etc/dbus-1/system.d/bluetooth.conf`:
```
  <policy group="plugdev">
    <allow send_destination="org.bluealsa"/>
  </policy>
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

## Configure bluealsa
`/usr/lib/systemd/system/bluealsa.service`:
```
...
ExecStart=/usr/bin/bluealsa --a2dp-force-audio-cd
```

## Allow powering off as user

`/usr/share/polkit-1/actions/org.freedesktop.login1.policy`:
```
        <action id="org.freedesktop.login1.power-off">
                <description gettext-domain="systemd">Power off the system</description>
                <message gettext-domain="systemd">Authentication is required for powering off the s$
                <defaults>
                        <allow_any>yes</allow_any>
                        <allow_inactive>yes</allow_inactive>
                        <allow_active>yes</allow_active>
                </defaults>
                <annotate key="org.freedesktop.policykit.imply">org.freedesktop.login1.set-wall-mes$
        </action>
```

## Enable the Apple SuperDrive at boot
`/etc/udev/rules.d/99-local.rules`:
```
# Initialise Apple SuperDrive
ACTION=="add", ATTRS{idProduct}=="1500", ATTRS{idVendor}=="05ac", DRIVERS=="usb", RUN+="/usr/bin/sg_raw /dev/$kernel EA 00 00 00 00 00 01"
```

## Install the [Buuf icon theme](https://www.gnome-look.org/p/1012512/)

`~/.config/gtk-3.0/settings.ini:`
```
[Settings]
gtk-icon-theme-name = buuf3.34
```

## Set last.fm API key
`~/.profile`:
```
...
export LAST_FM_API_KEY="YOUR_KEY_HERE"
```

## Run cdplayer at boot
`~/.xinitrc`:
```
cdplayer
```
`/etc/rc.local`:
```
...
su -l pi -c startx

exit 0
```
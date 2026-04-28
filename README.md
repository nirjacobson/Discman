# Discman
A CD player built from the [Raspberry Pi 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/), [Pi Display](https://shop.pimoroni.com/products/raspberry-pi-7-touchscreen-display-with-frame) and [Apple SuperDrive](https://www.apple.com/shop/product/MD564LL/A/apple-usb-superdrive).

Once assembled...

## Boot [discman-26.img](https://triton.nirjacobson.com/downloads/discman-26.img.xz)

This is a fully configured Triton OS image for Discman. Simply re-create the initramfs on first boot.

```
# dracut -fv
```
```
# nano /boot/config.txt
```
Uncomment this line:

`initramfs initramfs-6.11.10-v8+.img followkernel`
```
# reboot
```

## Configure an existing installation

### Install support packages
```
emerge -av sg3_utils udisks udiskie weston
```

### Install discman
```
emerge -av discman
```

### Configure ALSA for Bluetooth & CD audio
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

Configure bluez-alsa to force the audio CD sample rate (44.1 kHz):
```
# systemctl edit bluelsa.service
```
```
ExecStart=
ExecStart=/usr/bin/bluealsa -S -p a2dp-source -p a2dp-sink --a2dp-force-audio-cd
```

### Allow powering off as user

`/etc/polkit-1/rules.d/10-poweroff.rules`:
```
polkit.addRule(function(action, subject) {
    if (action.id == "org.freedesktop.login1.power-off" &&
        subject.user == "<your username>") {
        return polkit.Result.YES;
    }
});
```

### Allow udisks to automount for plugdev users

`/etc/polkit-1/rules.d/10-udisks2.rules`:
```
polkit.addRule(function(action, subject) {
	if (action.id.indexOf("org.freedesktop.udisks2.") == 0 &&
	    subject.isInGroup("plugdev")) {
		return polkit.Result.YES;
	}
});
```

### Set up udiskie service
`~/.config/systemd/user/udiskie.service`:
```
[Unit]
Description=udiskie automount service

[Service]
ExecStart=/usr/bin/udiskie --no-notify
Restart=always

[Install]
WantedBy=default.target
```

### Enable udiskie
```
$ systemctl --user enable udiskie
```

### Enable the Apple SuperDrive at boot
`/etc/udev/rules.d/99-local.rules`:
```
# Initialise Apple SuperDrive
ACTION=="add", ATTRS{idProduct}=="1500", ATTRS{idVendor}=="05ac", DRIVERS=="usb", RUN+="/usr/bin/sg_raw /dev/$kernel EA 00 00 00 00 00 01"
```

### Run Discman at boot
```
# systemctl edit getty@tty1.service

[Service]
Type=simple
ExecStart=
ExecStart=-/sbin/agetty -niJa discman %I 38400 linux
```

`~/.bash_profile`:
```
...

if tty | grep -q 'tty1'; then
    export DISCMAN_ALBUM_ART_PROVIDER="spotify" // Can be "lastfm" instead
    export SPOTIFY_CLIENT_ID=<your ID>          // Spotify client ID
    export SPOTIFY_CLIENT_SECRET=<your secret>  // Spotify client secret
    export LASTFM_API_kEY=<your key>            // last.fm API key (if using last.fm)

    sudo plymouthd --mode=boot --tty=$(tty)
    sudo plymouth show-splash
    sudo plymouth message --text="Waiting for Internet..."
    sleep 12
    sudo plymouth quit
    sudo plymouth --wait
    sleep 3
    weston --shell=kiosk-shell.so --xwayland > weston.log 2>&1 &
    export DISPLAY=:0.0
    sleep 1
    discman > discman.log 2>&1 &
fi
```
## Credits

 [Buuf icon theme](https://www.gnome-look.org/p/1012512/)

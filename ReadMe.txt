Calibration on linux (mint)

Check list of devives:
sudo evdev-joystick --l


Write a shell script in /usr/local/bin/calibrate-joystick.sh containing:

evdev-joystick --e /dev/input/by-id/usb-CRR_PicoWheel_83E7B5CADD8E1B08-event-joystick --a 0 --minimum 96 --maximum 3999 -d 32
evdev-joystick --e /dev/input/by-id/usb-CRR_PicoWheel_83E7B5CADD8E1B08-event-joystick --a 1 --minimum 96 --maximum 3999 -d 0
evdev-joystick --e /dev/input/by-id/usb-CRR_PicoWheel_83E7B5CADD8E1B08-event-joystick --a 2 --minimum 96 --maximum 3999 -d 0
evdev-joystick --e /dev/input/by-id/usb-CRR_PicoWheel_83E7B5CADD8E1B08-event-joystick --a 3 --minimum 96 --maximum 3999 -d 0
evdev-joystick --e /dev/input/by-id/usb-CRR_PicoWheel_83E7B5CADD8E1B08-event-joystick --a 4 --minimum 96 --maximum 3999 -d 0

Make the script executable by running:

sudo chmod +x /usr/local/bin/calibrate-joystick.sh

Create a udev rule in /etc/udev/rules.d/99-joystick-calibration.rules to run your script automatically whenever the device connects:

ACTION=="add", SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="CRR PicoWheelPHS", RUN+="/usr/local/bin/calibrate-joystick.sh '%E{DEVNAME}'"

Finally restart the system or run:

sudo udevadm control --reload-rules
sudo udevadm trigger

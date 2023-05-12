# mouse_switcher
Switch between a left and right handed mouse configuration easily

This program intercepts all events from your mouse and optionally swaps the left and rigth buttons before sending the event to the system. The swapping of the buttons can be enabled/disabled by pressing an additional mouse button.

## Configuration
The button to change the layout can be configured by editing the source code (redefine ``TOGGLE_BUTTON``).

## Installing
- Make sure you have [libevdev](https://www.freedesktop.org/software/libevdev/doc/latest/index.html) installed, on certain distros (e.g. Ubuntu) a dev package is required
- Clone this repository, build and install with
```
make
sudo make install
```

## Usage
Access to /dev/input/* and /dev/uinput is required, this can be achieved by running this as root. Basic usage:
```
mouse_switcher -e <eventfile>
```
Replace <eventfile> with the appropriate file for your keyboard in /dev/input (/dev/input/by-id makes finding the correct file easy).

Use the option -f to run in the background. Example:
```
mouse_switcher -f -e /dev/input/event0
```

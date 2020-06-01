# OBS Mute Indicator

This repo contains files to create a physical indicator for the "mute" state of an OBS Studio audio source. The state is automatically read by a Python script which then sends the data over USB (CDC serial) to an Arduino, which parses it and sets an indicator.

## Script Installation and Setup
The script only works with OBS Studio versions 21.x and later. If you have an older version you will need to update.

### Python Configuration
As of this writing OBS seems to have issues with the newest versions of Python (3.7+). This script was developed and tested using Python 3.6.8.

You need [Python 3.6](https://www.python.org/downloads/) installed on your PC. The bit version of your Python installation must match your OBS installation - use "x86-64" for 64 bit OBS Studio and "x86" for 32 bit OBS Studio. In the menu in OBS Studio, go to `Tools` and then `Scripts`. Then in the "Python Settings" tab, set the path to point to the Python installation folder.

This script relies on [the pySerial module](https://pypi.org/project/pyserial/) (3.0+) to handle serial bus communication. You can install pySerial using the following command:
```bash
python -m pip install pyserial
```

Add the mute indicator script to the "Scripts" window using the '+' icon on the bottom left. Select the script in the "Loaded Scripts" panel, and if everything is set up correctly you should see the script properties show up on the right.

### Script Options
Fill out the configuration settings in the script properties:

* **Audio Source**: the audio source to monitor for changes to its 'mute' status.
* **Serial Port**: the serial port to send state messages on.
* **Baudrate**: the baudrate to use when opening the serial connection.

The serial connection is opened automatically, either on script load or when serial settings have changed.

You can sending test 'mute' and 'unmute' messages by pressing the respective buttons. These buttons do *not* change the mute state of the audio input.

Output commands ('muted' / 'unmuted') are sent as utf-8 encoded strings and newline terminated.

## Arduino Setup

This repo contains three Arduino programs, each controlled through the OBS script and the USB serial interface. Each one renders the 'mute' data in a different way:

* **MuteIndicator**: blink an LED / toggle an output pin.
* **MuteStrip**: pulse an addressable LED strip.
* **MuteFlag**: use a servo to raise a physical flag and wave it around.

These programs were tested with an Arduino Uno (328P) and a Leonardo (32U4), allthough just about any Arduino should work. For each program, user-configurable options including pin assignments are listed at the top and commented.

## License
The contents of this repository are licensed under the terms of the [GNU General Public License v3.0](https://www.gnu.org/licens

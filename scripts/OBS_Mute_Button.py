#
# Project     OBS Mute Indicator Script
# @author     David Madison
# @link       github.com/dmadison/OBS-Mute-Indicator
# @license    GPLv3 - Copyright (c) 2020 David Madison
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import obspython as obs
import serial
import serial.tools.list_ports


# ------------------------------------------------------------

# Script Properties

debug = True  # default to "True" until overwritten by properties
source_name = ""  # source name to monitor, stored from properties
port_name = ""  # serial port, as device name
baudrate = 115200  # serial port baudrate
blink_speed = 500  # blink speed of LEDs in ms

# Constants

# ms before sending initial state (wait for device startup)
init_state_delay = 1600

# Global Variables

sources_loaded = False  # set to 'True' when sources are presumed loaded
callback_name = None  # source name for the current callback
port = None  # serial port object (PySerial)

# ------------------------------------------------------------

# Mute Indicator Functions


def dprint(*input):
    if debug == True:
        print(*input)


def open_port():
    global port

    if port_name == "Disconnected":
        return  # don't attempt to connect on the N/A option

    try:
        port = serial.Serial(port_name, baudrate, timeout=1)
        dprint("Opened serial port {:s} with baud {:d}".format(
            port_name, baudrate))

        if callback_name:  # if we have a 'mute' callback...
            # send the initial state
            obs.timer_add(send_initial_state, init_state_delay)

    except serial.serialutil.SerialException:
        dprint("ERROR: Could not open serial port", port_name)


def close_port():
    global port

    if not port:
        return  # nothing to close

    try:
        port.close()
        port = None  # remove port object
        dprint("Closed serial port {:s}".format(port_name))
    except serial.serialutil.SerialException:
        dprint("ERROR: Uhhh couldn't close the existing port {:s}? That's weird".format(
            port_name))


def write_output(muted):
    if not port:
        return  # no serial port

    output = "m" if muted else "u"
    output_terminated = output + str(blink_speed) + '\n'  # adding terminator

    try:
        port.write(output_terminated.encode('utf-8'))
    except serial.serialutil.SerialException:
        dprint("ERROR: Device on port {:s} not found".format(port_name))
    else:
        output = "\"{:s}\"".format(output)  # adding quotes
        dprint("Wrote: {:9s} to {:s} at {:d} baud".format(
            output, port_name, baudrate))


def write_reset():
    if not port:
        return  # no serial port

    output_terminated = 'r\n'

    try:
        port.write(output_terminated.encode('utf-8'))
    except serial.serialutil.SerialException:
        dprint("ERROR: Device on port {:s} not found".format(port_name))
    else:
        output = "\"{:s}\"".format(output)  # adding quotes
        dprint("Wrote: {:9s} to {:s} at {:d} baud".format(
            output, port_name, baudrate))


def send_initial_state():
    muted = get_muted(source_name)
    dprint("Sending initial state (startup delayed {:d} ms)".format(
        init_state_delay))
    write_output(muted)
    obs.remove_current_callback()  # remove this one-shot timer


def get_muted(sn):
    source = obs.obs_get_source_by_name(sn)

    if source is None:
        return None

    muted = obs.obs_source_muted(source)
    obs.obs_source_release(source)

    return muted


def set_muted(sn, state):
    source = obs.obs_get_source_by_name(sn)

    if source is None:
        return None

    obs.obs_source_set_muted(source, state)
    obs.obs_source_release(source)


def mute_callback(calldata):
    muted = obs.calldata_bool(calldata, "muted")  # true if muted, false if not
    write_output(muted)


def test_mute(prop, props):
    write_output(True)


def test_unmute(prop, props):
    write_output(False)


def create_muted_callback(sn):
    global callback_name

    if sn is None or sn == callback_name:
        return False  # source hasn't changed or callback is already set

    if callback_name is not None:
        remove_muted_callback(callback_name)

    source = obs.obs_get_source_by_name(sn)

    if source is None:
        if sources_loaded:  # don't print if sources are still loading
            dprint("ERROR: Could not create callback for", sn)
        return False

    handler = obs.obs_source_get_signal_handler(source)
    obs.signal_handler_connect(handler, "mute", mute_callback)
    callback_name = sn  # save name for future reference
    dprint("Added callback for \"{:s}\"".format(
        obs.obs_source_get_name(source)))

    if port:  # if the serial port is open...
        # send the initial state
        obs.timer_add(send_initial_state, init_state_delay)

    obs.obs_source_release(source)

    return True


def remove_muted_callback(sn):
    if sn is None:
        return False  # no callback is set

    source = obs.obs_get_source_by_name(sn)

    if source is None:
        dprint("ERROR: Could not remove callback for", sn)
        return False

    handler = obs.obs_source_get_signal_handler(source)
    obs.signal_handler_disconnect(handler, "mute", mute_callback)
    dprint("Removed callback for \"{:s}\"".format(
        obs.obs_source_get_name(source)))

    obs.obs_source_release(source)

    return True


def list_audio_sources():
    audio_sources = []
    sources = obs.obs_enum_sources()

    for source in sources:
        if obs.obs_source_get_type(source) == obs.OBS_SOURCE_TYPE_INPUT:
            # output flag bit field: https://obsproject.com/docs/reference-sources.html?highlight=sources#c.obs_source_info.output_flags
            capabilities = obs.obs_source_get_output_flags(source)

            has_audio = capabilities & obs.OBS_SOURCE_AUDIO
            # has_video = capabilities & obs.OBS_SOURCE_VIDEO
            # composite = capabilities & obs.OBS_SOURCE_COMPOSITE

            if has_audio:
                audio_sources.append(obs.obs_source_get_name(source))

    obs.source_list_release(sources)

    return audio_sources


def source_loading():
    global sources_loaded

    source = obs.obs_get_source_by_name(source_name)

    if source and create_muted_callback(source_name):
        sources_loaded = True  # sources loaded, no need for this anymore
        obs.remove_current_callback()  # delete this timer
    else:
        dprint("Waiting to load sources...")

    obs.obs_source_release(source)


def handle_button_press():
    global port

    if not port:
        return  # port is not open

    try:
        encoding = 'utf-8'
        button_state = port.read().decode(encoding)
        if(button_state == 'm'):
            set_muted(source_name, True)
            dprint(button_state)
        elif(button_state == 'u'):
            set_muted(source_name, False)
            dprint(button_state)
    except serial.serialutil.SerialException:
        dprint("ERROR: Device on port {:s} not found".format(port_name))


def flush_serial_input():
    global port

    if not port:
        return  # port is not open

    try:
        port.reset_input_buffer()  # clear input to prevent buffer overrun
    except serial.serialutil.SerialException:
        pass


# ------------------------------------------------------------

# OBS Script Functions

def script_description():
    return "<b>OBS Mute Indicator Script</b>" + \
        "<hr>" + \
        "Python script for sending the \"mute\" state of an audio source to a serial device." + \
        "<br/><br/>" + \
        "Made by David Madison, © 2020" + \
        "<br/>" + \
        "Modified by Stephan Beutel, © 2021" + \
        "<br/><br/>" + \
        "partsnotincluded.com" + \
        "<br/>" + \
        "github.com/dmadison/OBS-Mute-Indicator"


def script_update(settings):
    global debug, source_name, port_name, baudrate, port, blink_speed

    # for printing debug messages
    debug = obs.obs_data_get_bool(settings, "debug")

    new_port = obs.obs_data_get_string(
        settings, "port")  # serial device port name
    baudrate = obs.obs_data_get_int(settings, "baud")  # serial baud rate
    new_speed = obs.obs_data_get_int(
        settings, "blink_speed")  # blink speed in ms

    current_baud = None
    if port is not None:
        current_baud = port.baudrate

    if new_speed != blink_speed:
        blink_speed = new_speed

    if new_port != port_name or baudrate != current_baud:
        close_port()
        port_name = new_port
        open_port()

    source_name = obs.obs_data_get_string(settings, "source")

    if sources_loaded:
        # create 'muted' callback for source
        create_muted_callback(source_name)


def script_properties():
    props = obs.obs_properties_create()

    # Create list of audio sources and add them to properties list
    audio_sources = list_audio_sources()

    source_list = obs.obs_properties_add_list(
        props, "source", "Audio Source", obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_STRING)

    for name in audio_sources:
        obs.obs_property_list_add_string(source_list, name, name)

    # Create list of available serial ports
    port_list = obs.obs_properties_add_list(
        props, "port", "Serial Port", obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_STRING)

    obs.obs_property_list_add_string(
        port_list, "(Disconnect)", "Disconnected")  # include blank option at top
    com_ports = serial.tools.list_ports.comports()
    for port in com_ports:
        obs.obs_property_list_add_string(port_list, port.device, port.device)

    # Create a list of selectable baud rates
    baudrates = [115200, 57600, 38400, 31250, 28800,
                 19200, 14400, 9600, 4800, 2400, 1200, 600, 300]
    baud_list = obs.obs_properties_add_list(
        props, "baud", "Baudrate", obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_INT)

    for rate in baudrates:
        obs.obs_property_list_add_int(baud_list, str(rate), rate)

    # Create a list of selectable blink speeds
    blinkspeeds = [2000, 1000, 500, 200, 0]
    speed_list = obs.obs_properties_add_list(
        props, "blink_speed", "Blink Speed", obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_INT)

    for speed in blinkspeeds:
        obs.obs_property_list_add_int(speed_list, str(speed), speed)

    # Add testing buttons and debug toggle
    obs.obs_properties_add_button(
        props, "test_mute", "Test Mute Message", test_mute)
    obs.obs_properties_add_button(
        props, "test_unmute", "Test Unmute Message", test_unmute)

    obs.obs_properties_add_bool(props, "debug", "Print Debug Messages")

    return props


def script_load(settings):
    # brute force - try to load sources every 10 ms until the callback is created
    obs.timer_add(source_loading, 10)
    # read serial input buffer to get button action
    obs.timer_add(handle_button_press, 250)
    # flush serial input buffer every second to prevent buffer overruns
    obs.timer_add(flush_serial_input, 1000)
    dprint("OBS Mute Indicator Script Loaded")


def script_unload():
    write_reset()
    obs.timer_remove(source_loading)
    obs.timer_remove(handle_button_press)
    obs.timer_remove(send_initial_state)
    obs.timer_remove(flush_serial_input)

    close_port()  # close the serial port
    remove_muted_callback(callback_name)  # remove the callback if it exists
    dprint("OBS Mute Indicator Script Unloaded. Goodbye! <3")

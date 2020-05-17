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

debug = False  # default to "False" until overwritten by properties
port_name = ""  # serial port, as device name
baudrate = 115200  # serial port baudrate

# Global Variables

callback_name = None  # source name for the current callback
port = None  # serial port object (PySerial)

# ------------------------------------------------------------

# Mute Indicator Functions

def dprint(*input):
	if debug == True:
		print(*input)


def open_port():
	global port

	try:
		port = serial.Serial(port_name, baudrate, timeout=1)
		dprint("Opened serial port {:s} with baud {:d}".format(port_name, baudrate))
	except serial.serialutil.SerialException:
		dprint("ERROR: Could not open serial port", port_name)


def close_port():
	if port:
		dprint("Closed serial port {:s}".format(port_name))
		port.close()


def write_output(muted):
	if not port_name:
		return  # no serial port

	output = "muted" if muted else "unmuted"
	output_terminated = output + '\n'  # adding terminator

	try:
		port.write(output_terminated.encode('utf-8'))
	except serial.serialutil.SerialException:
		dprint("ERROR: Device on port {:s} not found".format(port_name))
	else:
		output = "\"{:s}\"".format(output)  # adding quotes
		dprint("Wrote: {:9s} to {:s} at {:d} baud".format(output, port_name, baudrate))


def mute_callback(calldata):
	muted = obs.calldata_bool(calldata, "muted")  # true if muted, false if not
	write_output(muted)


def test_mute(prop, props):
	write_output(True)


def test_unmute(prop, props):
	write_output(False)


def create_muted_callback(source_name):
	global callback_name

	if source_name is None or source_name == callback_name:
		return  # source hasn't changed and callback is already set

	if callback_name is not None:
		remove_muted_callback(callback_name)

	source = obs.obs_get_source_by_name(source_name)

	if source is None:
		dprint("ERROR: Could not create callback for", source_name)
		return

	handler = obs.obs_source_get_signal_handler(source)
	obs.signal_handler_connect(handler, "mute", mute_callback)
	callback_name = source_name  # save id for future reference
	dprint("Added callback for \"{:s}\"".format(obs.obs_source_get_name(source)))

	obs.obs_source_release(source)


def remove_muted_callback(source_name):
	if source_name is None:
		return  # no callback is set

	source = obs.obs_get_source_by_name(source_name)

	if source is None:
		dprint("ERROR: Could not remove callback for", source_name)
		return

	handler = obs.obs_source_get_signal_handler(source)
	obs.signal_handler_disconnect(handler, "mute", mute_callback)
	dprint("Removed callback for \"{:s}\"".format(obs.obs_source_get_name(source)))

	obs.obs_source_release(source)


def list_audio_sources():
	audio_sources = []
	sources = obs.obs_enum_sources()

	for source in sources:
		if obs.obs_source_get_type(source) == obs.OBS_SOURCE_TYPE_INPUT:
			# output flag bit field: https://obsproject.com/docs/reference-sources.html?highlight=sources#c.obs_source_info.output_flags
			capabilities = obs.obs_source_get_output_flags(source)

			has_video = capabilities & obs.OBS_SOURCE_VIDEO
			has_audio = capabilities & obs.OBS_SOURCE_AUDIO
			composite = capabilities & obs.OBS_SOURCE_COMPOSITE

			if has_audio and not has_video and not composite:
				audio_sources.append(obs.obs_source_get_name(source))

	obs.source_list_release(sources)

	return audio_sources


# ------------------------------------------------------------

# OBS Script Functions

def script_description():
	return "<b>OBS Mute Indicator Script</b>" + \
			"<hr>" + \
			"Python script for sending the \"mute\" state of an audio source to a serial device." + \
			"<br/><br/>" + \
			"Made by David Madison, © 2020" + \
			"<br/><br/>" + \
			"partsnotincluded.com" + \
			"<br/>" + \
			"github.com/dmadison/OBS-Mute-Indicator"


def script_update(settings):
	global debug, port_name, baudrate, port

	debug = obs.obs_data_get_bool(settings, "debug")  # for printing debug messages

	new_port = obs.obs_data_get_string(settings, "port")  # serial device port name
	baudrate = obs.obs_data_get_int(settings, "baud")  # serial baud rate

	current_baud = None
	if port is not None:
		current_baud = port.baudrate

	if new_port != port_name or baudrate != current_baud:
		close_port()
		port_name = new_port
		open_port()

	create_muted_callback(obs.obs_data_get_string(settings, "source"))  # create 'muted' callback for source


def script_properties():
	props = obs.obs_properties_create()

	# Create list of audio sources and add them to properties list
	audio_sources = list_audio_sources()

	source_list = obs.obs_properties_add_list(props, "source", "Audio Source", obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_STRING)

	for name in audio_sources:
		obs.obs_property_list_add_string(source_list, name, name)

	# Create list of available serial ports
	port_list = obs.obs_properties_add_list(props, "port", "Serial Port", obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_STRING)

	com_ports = serial.tools.list_ports.comports()
	for port in com_ports:
		obs.obs_property_list_add_string(port_list, port.device, port.device)

	# Create a list of selectable baud rates
	baudrates = [115200, 57600, 38400, 31250, 28800, 19200, 14400, 9600, 4800, 2400, 1200, 600, 300]
	baud_list = obs.obs_properties_add_list(props, "baud", "Baudrate", obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_INT)

	for rate in baudrates:
		obs.obs_property_list_add_int(baud_list, str(rate), rate)

	# Add testing buttons and debug toggle
	obs.obs_properties_add_button(props, "test_mute", "Test Mute Message", test_mute)
	obs.obs_properties_add_button(props, "test_unmute", "Test Unmute Message", test_unmute)

	obs.obs_properties_add_bool(props, "debug", "Print Debug Messages")

	return props


def script_load(settings):
	dprint("OBS Mute Indicator Script Loaded")


def script_unload():
	close_port()  # close the serial port
	remove_muted_callback(callback_name)  # remove the callback if it exists
	dprint("OBS Mute Indicator Script Unloaded. Goodbye! <3")

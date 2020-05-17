/*
 *  Project     OBS Mute Indicator
 *  @author     David Madison
 *  @link       github.com/dmadison/OBS-Mute-Indicator
 *  @license    GPLv3 - Copyright (c) 2020 David Madison
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


const uint8_t LED_Pin = LED_BUILTIN;

// Enumeration for getting the 'mute' output from the 
// parsed serial buffer
enum class MuteState {
	Unset = 0,
	Muted = 1,
	Unmuted = 2,
};


void setup() {
	Serial.begin(115200);

	pinMode(LED_Pin, OUTPUT);
}

void loop() {
	MuteState state = parseSerialMute();

	if (state != MuteState::Unset) {
		boolean muted = (state == MuteState::Muted);  // 1 if muted, 0 if not
		setMuted(muted);
	}
}

void setMuted(boolean muted) {
	digitalWrite(LED_Pin, muted);
}

MuteState parseSerialMute() {
	static const uint8_t BufferSize = 32;
	static char buffer[BufferSize];
	static uint8_t bIndex = 0;

	if (!Serial.available()) return MuteState::Unset;  // no data, no update

	// read serial characters into local buffer
	char c;
	while (true) {
		c = Serial.read();
		if (c == -1 || c == '\n') break;  // no byte available or terminator received
		buffer[bIndex++] = c;

		if (bIndex >= BufferSize - 1) {  // clear buffer if size exceeded (accounting for null terminator)
			memset(buffer, 0, BufferSize * sizeof(char));
			bIndex = 0;
		}
	}

	// if the terminating character is received, process the buffer
	if (c == '\n') {
		MuteState muteInput = MuteState::Unset;

		// check input vs strings
		if (strcasecmp("muted", buffer) == 0) muteInput = MuteState::Muted;
		else if (strcasecmp("unmuted", buffer) == 0) muteInput = MuteState::Unmuted;

		// clear buffer for next input string
		memset(buffer, 0, BufferSize * sizeof(char));
		bIndex = 0;

		return muteInput;
	}

	return MuteState::Unset;
}

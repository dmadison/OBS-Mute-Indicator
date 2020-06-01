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
 *
 *  Program:      MuteIndicator
 *  Description:  Listens for a "muted" (on) or "unmuted" (off) message on the
 *                serial interface. Sets an LED indicator light to blink
 *                accordingly.
 */

// User Options

const uint8_t LED_Pin = LED_BUILTIN;

const uint8_t Brightness = 255;  // 0 - 255, only if connected to a PWM pin
const unsigned long BlinkSpeed = 200;  // ms per blink period, '0' for no blinking
const boolean InvertOutput = false;

// ----------------------------------------------

// Enumeration for getting the 'mute' output from the 
// parsed serial buffer
enum class MuteState {
	Unset = 0,
	Muted = 1,
	Unmuted = 2,
};

boolean lightOn = false;  // flag to track the 'on' state of the light
boolean ledState = LOW;  // state of the output LED, high or low
unsigned long lastBlink = 0;  // ms, last time the blink switched


void setup() {
	Serial.begin(115200);

	pinMode(LED_Pin, OUTPUT);
	setLED(LOW); // turn off initially
}

void loop() {
	MuteState state = parseSerialMute();

	if (state != MuteState::Unset) {
		lightOn = (state == MuteState::Muted);  // 'true' if muted, 'false' if not
		setLED(lightOn);  // set LED to the output state
		lastBlink = millis();  // reset flash timer
	}

	if (lightOn == true) blinkLED();  // if the light is enabled, blink
}

void setLED(boolean state) {
	ledState = state;  // save state for later reference
	if (InvertOutput) state = !state;  // if inverting, flip output state
	
	if (state == true) analogWrite(LED_Pin, Brightness);  // ON = PWM for brightness
	else digitalWrite(LED_Pin, LOW);  // OFF
}

void blinkLED() {
	if (BlinkSpeed == 0 || millis() - lastBlink < BlinkSpeed / 2) return;  // no blinking (at least not yet)

	lastBlink = millis();  // save timestamp for next time
	ledState = !ledState;  // flip output
	setLED(ledState);
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

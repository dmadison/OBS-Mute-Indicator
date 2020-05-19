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
 *  Program:      MuteFlag
 *  Description:  Listens for a "muted" (on) or "unmuted" (off) message on the
 *                serial interface. If "muted", uses a servo motor to wave
 *                a physical flag as an indicator.
 */

#include <Servo.h>

 // User Options
const uint8_t ServoPin = 6;

const uint8_t WaveMinAngle = 120;  // degrees, 0 - 180
const uint8_t WaveMaxAngle = 180;

const uint16_t ServoMinPulse = 550;   // us, min pulse time (angle 0)
const uint16_t ServoMaxPulse = 1600;  // us, max servo pulse time (angle 180)
const boolean  InvertServo = true;   // set to 'true' to invert the servo output

const unsigned long WriteDelay = 5000;  // us, wait for each servo write
const unsigned long WaveDwellTime = 150;  // ms, time to dwell on either side of the wave

// ----------------------------------------------

// Enumeration for getting the 'mute' output from the 
// parsed serial buffer
enum class MuteState {
	Unset = 0,
	Muted = 1,
	Unmuted = 2,
};

enum class FlagState {
	Raising,
	Waving,
	Lowering,
	Down,
};

Servo servo;

FlagState fState = FlagState::Down;
int16_t fPosition = 0;  // flag position, in degrees (0 - 180)
int8_t waveDirection = 1;  // direction the flag is moving while 'waving'.


void setup() {
	Serial.begin(115200);

	servo.attach(ServoPin);
	writeAngle(fPosition);   // write initial position
	delay(500);  // wait for initial position to set
	servo.detach();  // free servo until it's needed
}

void loop() {
	MuteState state = parseSerialMute();

	if (state == MuteState::Muted) {
		if (fState == FlagState::Down) servo.attach(ServoPin);  // reattach if disconnected
		fState = FlagState::Raising;
		waveDirection = 1;
	}
	else if (state == MuteState::Unmuted && fState != FlagState::Down) {
		fState = FlagState::Lowering;
	}

	if (fState != FlagState::Down) {
		switch (fState) {
		case(FlagState::Raising):  // raise the flag, one degree at a time until max
			writeAngle(fPosition++);
			if (fPosition >= WaveMinAngle) fState = FlagState::Waving;  // hit top, start waving
			break;
		case(FlagState::Lowering):  // lower the flag, one degree at a time until min
			writeAngle(fPosition--);
			if (fPosition <= 0) fState = FlagState::Down;  // hit bottom, turn off
			break;
		case(FlagState::Waving):
			waveFlag();
			break;
		default:
			break;  // catch 'down'
		}

		if(fState == FlagState::Down) servo.detach();  // if we're down, stop sending signals to the servo
	}
}

void waveFlag() {
	fPosition += waveDirection;
	writeAngle(fPosition);

	if (fPosition >= WaveMaxAngle || fPosition <= WaveMinAngle) {
		delay(WaveDwellTime);
		waveDirection = -waveDirection;
	}
}

void writeAngle(uint8_t deg) {
	if (deg > 180) deg = 180;

	long pulse;
	if (InvertServo == true) pulse = map(deg, 0, 180, ServoMaxPulse, ServoMinPulse);
	else pulse = map(deg, 0, 180, ServoMinPulse, ServoMaxPulse);

	servo.writeMicroseconds(pulse);
	delayMicroseconds(WriteDelay);
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

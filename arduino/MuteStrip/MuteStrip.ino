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
 *  Program:      MuteStrip
 *  Description:  Listens for a "muted" (on) or "unmuted" (off) message on the
 *                serial interface. If a "muted" message is received, pulses
 *                an addressable LED strip to alert the user.
 */

#include <FastLED.h>

 // User Options
const uint8_t NumLEDs = 7;
const uint8_t MinBrightness = 32;
const uint8_t MaxBrightness = 128;  // 0 - 255, gamma 2.2

const uint8_t Framerate = 60;  // frames per second, for smooth stepping
const unsigned long PulseSpeed = 1000;  // ms per period
const CRGB PulseColor = CRGB::Red;  // color when the LEDs are on

#define LED_TYPE WS2812B
#define DATA_PIN 6
// #define CLOCK_PIN 13
#define COLOR_ORDER GRB

// ----------------------------------------------

// Enumeration for getting the 'mute' output from the 
// parsed serial buffer
enum class MuteState {
	Unset = 0,
	Muted = 1,
	Unmuted = 2,
};

CRGB leds[NumLEDs];

boolean lightOn = false;  // flag to track the 'on' state of the light

const uint8_t NumPulseSteps = (PulseSpeed / 2) / (1000 / Framerate);  // number of steps in each direction per pulse
const uint8_t PulseSize = (MaxBrightness - MinBrightness) / NumPulseSteps;  // amount to increment pulse brightness each time

int16_t pulseBrightness = 0;  // pulse brightness, 0 - 255  (int16 to account for overflow)
int8_t  pulseDirection = 1;   // direction for the pulse, 1 or -1

unsigned long lastUpdate = 0;  // ms, last time the LEDs were updated (framerate limit)

/* Gamma table (2.2), a modified version from the one courtesy of Adafruit
 * The 2.8 table provided looked a little peaky. This is smoother while retaining
 * the same advantage of linear perception.
 *
 *  Reference: https://learn.adafruit.com/led-tricks-gamma-correction/
 */
const uint8_t PROGMEM gamma8[] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
	3,  3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,
	6,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 11, 11, 11, 12,
   12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19,
   20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29,
   30, 30, 31, 32, 33, 33, 34, 35, 35, 36, 37, 38, 39, 39, 40, 41,
   42, 43, 43, 44, 45, 46, 47, 48, 49, 49, 50, 51, 52, 53, 54, 55,
   56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
   73, 74, 75, 76, 77, 78, 79, 81, 82, 83, 84, 85, 87, 88, 89, 90,
   91, 93, 94, 95, 97, 98, 99,100,102,103,105,106,107,109,110,111,
  113,114,116,117,119,120,121,123,124,126,127,129,130,132,133,135,
  137,138,140,141,143,145,146,148,149,151,153,154,156,158,159,161,
  163,165,166,168,170,172,173,175,177,179,181,182,184,186,188,190,
  192,194,196,197,199,201,203,205,207,209,211,213,215,217,219,221,
  223,225,227,229,231,234,236,238,240,242,244,246,248,251,253,255
};

// Adding comma to the CLOCK_PIN definition if provided
#ifdef CLOCK_PIN
	#define CLOCK_PIN_C CLOCK_PIN,
#else
	#define CLOCK_PIN_C
#endif


void setup() {
	Serial.begin(115200);

	fill_solid(leds, NumLEDs, PulseColor);  // other color changes will be done with brightness only

	FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN_C COLOR_ORDER>(leds, NumLEDs);
	resetLEDs();  // clear to start
}

void loop() {
	MuteState state = parseSerialMute();

	if (state != MuteState::Unset) {
		lightOn = (state == MuteState::Muted);  // 'true' if muted, 'false' if not
		if (lightOn == false) resetLEDs();  // if turning off, clear LEDs
	}

	if (lightOn == true) pulseLEDs();
}

void pulseLEDs() {
	if (millis() - lastUpdate < (1000 / Framerate)) return;  // framerate limiter
	lastUpdate = millis();  // save timestamp

	pulseBrightness += PulseSize * pulseDirection;  // increment step

	if (pulseBrightness >= MaxBrightness) {
		pulseBrightness = MaxBrightness;
		pulseDirection = -pulseDirection;
	}
	else if (pulseBrightness <= MinBrightness) {
		pulseBrightness = MinBrightness;
		pulseDirection = -pulseDirection;
	}

	uint8_t gamma = getGammaBrightness((uint8_t) pulseBrightness);
	FastLED.setBrightness(gamma);
	
	FastLED.show();
}

void resetLEDs() {
	pulseBrightness = 0;  // start 'off'
	pulseDirection = 1;  // rising pulse

	FastLED.setBrightness(0);  // clear
	FastLED.show();
}

uint8_t getGammaBrightness(uint8_t bright) {
	return pgm_read_byte(&gamma8[bright]);
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

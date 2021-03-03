/*
    Project     OBS Mute Indicator
    @author     David Madison, Stephan Beutel
    @link       github.com/dmadison/OBS-Mute-Indicator
    @license    GPLv3 - Copyright (c) 2020 David Madison

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Program:      MuteIndicator
    Description:  Listens for a "muted" (on) or "unmuted" (off) message on the
                  serial interface. Sets an LED indicator light to blink
                  accordingly.
*/

// User Options
#include <Adafruit_NeoPixel.h>

#define PIN 10
int numPixels = 12;

const int buttonPin = 2;

unsigned long blinkSpeed = 500; // ms per blink period, '0' for no blinking
const boolean InvertOutput = false;

// Button
int lastState = LOW; // the previous state from the input pin
int currentState;    // the current reading from the input pin
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
boolean pressed = false;

// RGB Ring
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, PIN, NEO_GRB + NEO_KHZ800);
uint32_t red = strip.Color(255, 0, 0);   //RGB Farbe Rot
uint32_t green = strip.Color(0, 255, 0); //RGB Farbe Grün

uint8_t brightness = 100; // Initialwert für die Helligkeit (0 bis 50)
uint16_t innerColorLoop = 0, outerColorLoop = 0;

// ----------------------------------------------

// Enumeration for getting the 'mute' output from the
// parsed serial buffer
enum class MuteState
{
  Unset = 0,
  Muted = 1,
  Unmuted = 2,
};

boolean lightOn = false;     // flag to track the 'on' state of the light
boolean ledState = true;     // state of the output LED, high or low
unsigned long lastBlink = 0; // ms, last time the blink switched

MuteState parsedState;

void setup()
{
  Serial.begin(115200);

  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPressDetected, FALLING);

  pinMode(buttonPin, INPUT);

  // RGB Ring
  strip.begin();
  strip.setBrightness(brightness); //die Helligkeit setzen 0 dunke -> 255 ganz hell
  strip.show();                    // Alle NeoPixel sind im Status "aus".
}

void loop()
{

  if (parsedState != MuteState::Unset)
  {
    lightOn = (parsedState == MuteState::Unmuted);                                             // 'true' if muted, 'false' if not
    lightRgbRing(lightOn ? red : green, lightOn ? ledState ? brightness : 0 : brightness / 2); // set LED to the output state
  }
  else
  {
    rainbowCycle();
  }

  if (lightOn == true)
    blinkLED(); // if the light is enabled, blink
}

void blinkLED()
{
  if (blinkSpeed == 0 || millis() - lastBlink < blinkSpeed / 2)
    return; // no blinking (at least not yet)

  lastBlink = millis(); // save timestamp for next time
  ledState = !ledState; // flip output
}

void lightRgbRing(uint32_t color, uint8_t state)
{
  uint16_t i, j;
  for (i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, color);
    strip.setBrightness(state);
  }
  strip.show();
}

void rainbowCycle()
{
  uint16_t i, j;
  for (j = 0; j < 256; j++)
  { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, ColorWheel(((i * 256 / strip.numPixels()) + j) & 255));
      strip.setBrightness(map(brightness, 0, 50, 5, 255));
    }
    strip.show();
    delay(blinkSpeed / 100);
  }
}

/************************( Funktion um RGB Farben zu erzeugen )***************************/
uint32_t ColorWheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void buttonPressDetected()
{
  pressed = true;
  if (parsedState == MuteState::Unmuted)
    parsedState = MuteState::Muted;
  else
    parsedState = MuteState::Unmuted;

  switch (parsedState)
  {
  case MuteState::Unmuted:
    Serial.println("u");
    break;
  case MuteState::Muted:
    Serial.println("m");
    break;
  default:
    break;
  }
}

void serialEvent()
{
  static const uint8_t BufferSize = 32;
  static char buffer[BufferSize];
  static uint8_t bIndex = 0;

  MuteState muteInput = MuteState::Unset;

  if (Serial.available())
  {

    // read serial characters into local buffer
    char c;
    while (true)
    {
      c = Serial.read();
      if (c == -1 || c == '\n')
        break; // no byte available or terminator received
      buffer[bIndex++] = c;

      if (bIndex >= BufferSize - 1)
      { // clear buffer if size exceeded (accounting for null terminator)
        memset(buffer, 0, BufferSize * sizeof(char));
        bIndex = 0;
      }
    }

    // if the terminating character is received, process the buffer
    if (c == '\n')
    {
      boolean brightSet = false;
      // check input vs strings
      if (strpbrk(buffer, "m") != NULL)
        muteInput = MuteState::Muted;
      else if (strpbrk(buffer, "u") != NULL)
        muteInput = MuteState::Unmuted;

      blinkSpeed = atoi(buffer + 1);

      // clear buffer for next input string
      memset(buffer, 0, BufferSize * sizeof(char));
      bIndex = 0;
    }
  }
  parsedState = muteInput;
}

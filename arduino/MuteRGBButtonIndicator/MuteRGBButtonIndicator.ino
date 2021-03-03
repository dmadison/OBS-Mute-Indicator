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
#include <Adafruit_NeoPixel.h>

// User Options

#define PIN 10 // pin to control RGB ring
int numPixels = 12;
const int buttonPin = 2;
unsigned long blinkSpeed = 500; // ms per blink period, '0' for no blinking
uint8_t brightness = 150;       // Initialwert für die Helligkeit (0 bis 50)

// RGB Ring
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, PIN, NEO_GRB + NEO_KHZ800);

uint32_t unmutedColor = strip.Color(255, 0, 0); //RGB color Unmuted, red
uint32_t mutedColor = strip.Color(0, 255, 0);   //RGB color Mute, green

static unsigned long last_interrupt_time = 0;
static boolean initialized = false;

// ----------------------------------------------

// Enumeration for getting the 'mute' output from the
// parsed serial buffer
enum class MuteState
{
  Unset = 0,
  Muted = 1,
  Unmuted = 2,
};

MuteState parsedState;

int j = 1;
int direction = 1;

void setup()
{
  Serial.begin(115200);

  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPressDetected, FALLING);

  pinMode(buttonPin, INPUT_PULLUP);

  // RGB Ring
  strip.begin();
  strip.setBrightness(brightness); // set initial brightness 0 dark -> 255 bright
  strip.show();                    // all leds off.
}

void loop()
{
  if (parsedState != MuteState::Unset)
  {
    if (parsedState == MuteState::Unmuted) // 'true' if muted, 'false' if not
    {
      strip.setBrightness(brightness);
      pulseUnmutedRed();
    }
    else
    {
      strip.setBrightness(brightness / 3);
      lightRgbRing(mutedColor);
    }
  }
  else
  {
    rainbowCycle();
  }
}

void pulseUnmutedRed()
{
  int color = 180 - 10 * j;
  for (int i = 0; i < numPixels; i++)
  {
    strip.setPixelColor(i, strip.Color(color, 0, 0));
    strip.show();
  }
  delay(blinkSpeed / 10);
  if (j == 11)
  {
    direction = -1;
  }
  else if (j == 0)
  {
    direction = 1;
  }

  j = j + direction;
}

void lightRgbRing(uint32_t color)
{
  uint16_t i, j;
  for (i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, color);
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
    }
    strip.show();
    delay(blinkSpeed / 50);
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
  if (initialized)
  {
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 1000)
    {
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
    last_interrupt_time = interrupt_time;
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

      if (!initialized)
        initialized = true;
    }
  }
  parsedState = muteInput;
}

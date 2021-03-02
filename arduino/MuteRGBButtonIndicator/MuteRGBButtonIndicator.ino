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

const int redLedPins[] = {6, 7, 8, 9};
const int greenLedPins[] = {4, 5};
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

int Helligkeit = 10;  // Initialwert für die Helligkeit (0 bis 50)
int SpeedLedMap = 10; // Initialwert für die Skalierte Geschwindigkeit
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
boolean ledState = LOW;      // state of the output LED, high or low
unsigned long lastBlink = 0; // ms, last time the blink switched

int delayTime = 5000; // TESTING
MuteState parsedState;

void setup()
{
  Serial.begin(115200);

  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPressDetected, CHANGE);

  int index;
  for (index = 0; index <= 3; index++)
  {
    pinMode(redLedPins[index], OUTPUT);
  }
  for (index = 0; index <= 1; index++)
  {
    pinMode(greenLedPins[index], OUTPUT);
  }

  pinMode(buttonPin, INPUT);

  setRedLED(LOW);    // turn off initially
  setGreenLED(HIGH); // turn off initially

  // RGB Ring
  strip.begin();
  strip.setBrightness(10); //die Helligkeit setzen 0 dunke -> 255 ganz hell
  strip.show();            // Alle NeoPixel sind im Status "aus".
}

void loop()
{
  handleButtonState();
  parsedState = parseSerialMute();

  if (parsedState != MuteState::Unset)
  {
    lightOn = (parsedState == MuteState::Unmuted); // 'true' if muted, 'false' if not
    setRedLED(lightOn);                            // set LED to the output state
    setGreenLED(!lightOn);                         // set LED to the output state
    lastBlink = millis();                          // reset flash timer
  }

  if (lightOn == true)
    blinkLED(); // if the light is enabled, blink

  lightRgbRing();
}

void lightRgbRing()
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {                                //Für jeden NeoPixel
    strip.setPixelColor(i, red);   // Farbe Rot setzen
    strip.show();                  //Anzeigen
    delay(100);                    //Warten von 1 sek.
    strip.setPixelColor(i, green); //Farbe Grün setzen
    strip.show();                  //Anzeigen
                                   //Wenn noch NeoPixel folgen dann den Wert "i" um 1 erhöhen und von vorne beginnen wenn nicht Schleife beenden.
  }
}

void rainbowCycle()
{
  if (outerColorLoop == 256 * 5)
  {
    outerColorLoop = 0;
  }
  for (innerColorLoop; innerColorLoop < strip.numPixels(); innerColorLoop++)
  {
    strip.setPixelColor(innerColorLoop, Wheel(((innerColorLoop * 256 / strip.numPixels()) + outerColorLoop) & 255));
    strip.setBrightness(map(Helligkeit, 0, 50, 5, 255));
  }
  strip.show();
  delay(SpeedLedMap);

  outerColorLoop++;
}

/************************( Funktion um RGB Farben zu erzeugen )***************************/
uint32_t Wheel(byte WheelPos)
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

void setRedLED(boolean state)
{
  ledState = state; // save state for later reference
  if (InvertOutput)
    state = !state; // if inverting, flip output state

  int index;

  for (index = 0; index <= 3; index++)
  {
    if (state == true)
      digitalWrite(redLedPins[index], HIGH); // ON = PWM for brightness
    else
      digitalWrite(redLedPins[index], LOW); // OFF
  }
}

void setGreenLED(boolean state)
{
  ledState = state; // save state for later reference
  if (InvertOutput)
    state = !state; // if inverting, flip output state

  int index;

  for (index = 0; index <= 1; index++)
  {
    if (state == true)
      digitalWrite(greenLedPins[index], HIGH); // ON = PWM for brightness
    else
      digitalWrite(greenLedPins[index], LOW); // OFF
  }
}

void blinkLED()
{
  if (blinkSpeed == 0 || millis() - lastBlink < blinkSpeed / 2)
    return; // no blinking (at least not yet)

  lastBlink = millis(); // save timestamp for next time
  ledState = !ledState; // flip output
  setRedLED(ledState);
}

void handleButtonState()
{
  // read the state of the switch/button:
  currentState = digitalRead(buttonPin);

  if (lastState == HIGH && currentState == LOW)
  { // button is released
    pressed = true;
    Serial.println("Pressed");
  }

  // save the the last state
  lastState = currentState;
}

void buttonPressDetected()
{
  pressed = true;
}

MuteState parseSerialMute()
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
      muteInput = MuteState::Unset;

      // check input vs strings
      if (strpbrk(buffer, "m") != NULL)
        muteInput = MuteState::Muted;
      else if (strpbrk(buffer, "m") == NULL)
        muteInput = MuteState::Unmuted;

      blinkSpeed = atoi(buffer + 1);

      // clear buffer for next input string
      memset(buffer, 0, BufferSize * sizeof(char));
      bIndex = 0;

      return muteInput;
    }
  }

  if (pressed)
  {
    pressed = false;
    muteInput = MuteState::Unset;
    Serial.print("pressed");
    // toggle mute state:
    if (lightOn)
    {
      muteInput = MuteState::Muted;
    }
    else
    {
      muteInput = MuteState::Unmuted;
    }
    switch (muteInput)
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
    return muteInput;
  }

  return muteInput;
}

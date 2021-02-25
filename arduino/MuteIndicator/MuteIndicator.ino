/*
 *  Project     OBS Mute Indicator
 *  @author     David Madison, Stephan Beutel
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

int redLedPins[] = {6, 7, 8, 9};
int greenLedPins[] = {4, 5};

unsigned long blinkSpeed = 500; // ms per blink period, '0' for no blinking
const boolean InvertOutput = false;

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
  int index;
  for (index = 0; index <= 3; index++)
  {
    pinMode(redLedPins[index], OUTPUT);
  }
  for (index = 0; index <= 1; index++)
  {
    pinMode(greenLedPins[index], OUTPUT);
  }

  setRedLED(LOW);    // turn off initially
  setGreenLED(HIGH); // turn off initially
}

void loop()
{
  parsedState = parseSerialMute();

  //testing(); //TESTING

  if (parsedState != MuteState::Unset)
  {
    lightOn = (parsedState == MuteState::Unmuted); // 'true' if muted, 'false' if not
    setRedLED(lightOn);                            // set LED to the output state
    setGreenLED(!lightOn);                         // set LED to the output state
    lastBlink = millis();                          // reset flash timer
  }

  if (lightOn == true)
    blinkLED(); // if the light is enabled, blink
}

void testing()
{
  parsedState = MuteState::Unmuted;
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

MuteState parseSerialMute()
{
  static const uint8_t BufferSize = 32;
  static char buffer[BufferSize];
  static uint8_t bIndex = 0;

  if (!Serial.available())
    return MuteState::Unset; // no data, no update

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
    MuteState muteInput = MuteState::Unset;

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

  return MuteState::Unset;
}

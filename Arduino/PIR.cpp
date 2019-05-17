/*!
 * @file PIR.cpp
 * 
 * @mainpage Arduino library handling a PIR sensor
 * 
 * @section intro_sec Introduction
 * 
 * This class is used to manage a PIR sensor. It attempts to mitigate
 * some of the glitches seen in some PIR sensors.
 * 
 * The class also handles lighting an indicator LED on an externally
 * defined neopixel strip. Indicator index in strip is defined at 
 * constructor time.
 *
 * @section author Author
 * 
 * Written by Jim Calvin.
 * 
 * @section license License
 * 
 * This file is part of the project Stairway, an Arduino sketch to light
 * a stairway using two PIR sensors and a NeoPixel strip. This software
 * is free software. You can redistribute it and/or modify it under the 
 * terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation, either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * PIR.cpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#include "Arduino.h"
#include "PIR.h"
#include <Adafruit_NeoPixel.h>

unsigned long stableStateMinimum = 100; // milliseconds

/************************************************************************************
 * read() includes code to simulate a PIR being connected when debugging is enabled
 ************************************************************************************/
PIR::PIR(int pin, int indicatorIndex, const char *name, bool debug) {
  _debug = debug;
  if (_debug) {
    pinMode(pin, INPUT_PULLUP);
  } else {
    pinMode(pin, INPUT);
  }
  PIRName = name;
  _pin = pin;
  _indicatorIndex = indicatorIndex;
  _PIRTransitionTime = 0;
  _reportedState = false;
}

// raw state of the PIR
bool PIR::readRaw() {
  bool state = digitalRead(_pin) == HIGH;
  if (_debug) { state = !state; }
  return state;
}

// normally used to read PIR state
bool PIR::read() {
  bool state = readRaw();
  unsigned long now = millis();
// reflect current state in indicators
  if (state != _previousState) {     // show changes whether we report them or not
    uint32_t color = (state) ? indicatorColor : offColor;
    pixels.setPixelColor(_indicatorIndex, color);
    pixels.show();
    _PIRTransitionTime = now;
    _previousState = state;
  }
// now determine what to report (debounce too)
  if ((now - _PIRTransitionTime) > stableStateMinimum) {
    _PIRTransitionTime = now;
    _reportedState = state;
  }
  return _reportedState;
}

bool PIR::debugMode() {
  return _debug;
}

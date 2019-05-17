/*!
 * @file PIR.h
 * 
 * @mainpage Arduino library handling a PIR sensor
 * 
 * @section intro_sec Introduction
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
 * PIR.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#include "Arduino.h"

#ifndef PIR_h
#define PIR_h

#include <Adafruit_NeoPixel.h>
#include "AnimationGlobals.h"

/************************************************************************************
 * read() includes code to simulate a PIR being connected when debugging is enabled
 ************************************************************************************/
class PIR
{
  public:
    PIR(int pin, int indicatorIndex, const char *name, bool debug=false);
    bool read();                  // normal read function
    bool readRaw();               // returns current state, no filtering
    bool debugMode();             // returns debug setting
    const char *PIRName;

  private:
    unsigned long _PIRTransitionTime;
    int _pin;                     // pin PIR is connected to
    bool _reportedState;          // state from last time we read this PIR
    bool _previousState;          // internally used for debounce
    int _indicatorIndex;          // LED strip index for indicator; <0 says no indicator
    bool _debug;
};

#endif

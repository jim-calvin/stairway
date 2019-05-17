/*!
 * @file Twinkle.h
 * 
 * @mainpage Arduino library for randomly turn on LEDs in a NeoPixel strip
 * and once all are on, randomly turning a few LEDs off and on.
 * 
 * @section intro_sec Introduction
 * 
 * This class is derived from Animation and randomly turn on LEDs in a NeoPixel strip
 * Once all are on, randomly selected LEDs are turned off and on to create a
 * twinkling like effect.
 * 
 * Finish() randomly turns off LEDs until all are off.
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
 * Twinkle.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "Animation.h"
#include "AnimationGlobals.h"

#ifndef Twinkle_h
#define Twinkle_h

/************************************************************************************
 * Start():     initiates randomly lighting LEDs with a random color
 * Continue();  continues randomly lighting LEDs until all are ON
 *              then randomly chooses a few LEDs to turn off and on
 * Finish():    intiates randomly turning off random LEDs, Continue()
 *              keeps this process going until all LEDs are OFF
 ************************************************************************************/
class Twinkle : public Animation {
  public:
    Twinkle(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();

  protected:
    int _MAXTOTWINKLE = kNumberOfLEDs/10;
    int _twinkleIndices[kNumberOfLEDs/10];

  private:
    bool _LEDArray[kNumberOfLEDs];       // array containing on/off state of each LED
    unsigned long _twinkleLongestTimeToWait;
    int _twinkleChangedCount;
    int _twinkleIdx;
    int _twinkleState;                  // whether we're turning LEDs on, twinkling them, or turning them off
    bool _twinkleToOn;

    void twinkleFinish(bool desiredState);
    void twinkleSomeInit();
    void twinkleSomeLEDs();
    void commonTwinkleInitiate();
    bool indexInIndices(int newIdx);
};

#endif

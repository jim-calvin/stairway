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
#include "Twinkle.h"

#define twinkleTurningOn 0        // we're turning on LEDs
#define twinkleTwinkling 1        // we're twinkling LEDs
#define twinkleTurningOff 2       // we're turing LEDs off

/************************************************************************************
 * Start():     initiates randomly lighting LEDs with a random color
 * Continue();  continues randomly lighting LEDs until all are ON
 *              then randomly chooses a few LEDs to turn off and on
 * Finish():    intiates randomly turning off random LEDs, Continue()
 *              keeps this process going until all LEDs are OFF
 ************************************************************************************/

// helper functions

// sometimes we're finished, but not all LEDs have been transitioned to a color or OFF
// we clean that up here & maybe initialize twinkling
void Twinkle::twinkleFinish(bool desiredState) {    // wrap things up (maybe)
  if (!desiredState) {                              // want things OFF?
    setAllPixelsTo(offColor);                       // do that & done here
    return;
  }
// loop through pixels to see if any are not ON & pick a color and turn them on
  for (int i=_firstLED; i<=_lastLED; i++) {
    if (_LEDArray[i] != desiredState) {
      uint32_t aColor = randomColor();
      if (!desiredState) {
        aColor = offColor;
      }
      pixels.setPixelColor(i, aColor);
    }
  }
  pixels.show();                                  // finally, show the changes
}

// setup to twinkle a few of the LEDs
void Twinkle::twinkleSomeInit() {
  for (int i=0; i<_MAXTOTWINKLE; i++) {            // choose some LEDs to twinkle
    _twinkleIndices[i] = random(_firstLED, _lastLED-1);
  }
  _twinkleState = twinkleTwinkling;               // set the mode
  _twinkleToOn = false;                           // start by turning LEDs off
  _twinkleIdx = 0;                                // reset our poiner
}

bool Twinkle::indexInIndices(int newIdx) {        // check to see if an LED we're about to choose
  for (int i=0; i<_MAXTOTWINKLE; i++) {           // already exists in the twinkling set of LEDs
    if (_twinkleIndices[i] == newIdx) {
      return true;                                // inform caller, which find a different LED to use
    }
  }
  return false;
}

// twinkle (turn off then back on) a few LEDs ~10% of strip
void Twinkle::twinkleSomeLEDs() {
  if (int(millis()-_lastUpdateTime) < (_animationStepIncrement*2)) {  // is it time?
    return;                                       // no, nothing to do
  }
  uint32_t aColor = offColor;                     // assume turn off
  if (_twinkleToOn) {                             // but we're turning it back on
    aColor = randomColor();                       // get a new color
  }
  pixels.setPixelColor(_twinkleIndices[_twinkleIdx], aColor);
  pixels.show();                                 // force the change to show
  if (_twinkleToOn) {                            // if back on, change the pixel we'll do next time
    int newIdx;
    do {
      newIdx = random(_firstLED, _lastLED+1);
    } while (indexInIndices(newIdx));
    _twinkleIndices[_twinkleIdx] = newIdx;
  }
  _twinkleIdx += 1;                             // walk over our "buffer"
  if (_twinkleIdx >= _MAXTOTWINKLE) {
    _twinkleIdx = 0;       // wrap around
    _twinkleToOn = !_twinkleToOn;               // and change on/off modes
  }
  _lastUpdateTime = millis();
}

// helper function used by Start and Finish
void Twinkle::commonTwinkleInitiate() {
  pixels.setBrightness(255);
  _twinkleLongestTimeToWait = int(round(_animationTime*1000))+millis(); // max time for pattern
  _twinkleIdx = 0;
  _twinkleChangedCount = 0;                       // none changed yet
}

/************************************************************************************
 * Constructor for Twinkle
 ************************************************************************************/
// Initialize our instance, base class handles it
Twinkle::Twinkle(const char * animationName, float animationTime, int firstOffset, int lastOffset):Animation(animationName, animationTime, firstOffset, lastOffset) {
}

/************************************************************************************
 * Start():     initiates randomly lighting LEDs with a random color
 ************************************************************************************/
void Twinkle::Start(bool topToBottom, uint32_t colorToUse) {
  setAllPixelsTo(offColor);
  pixels.show();
  _topToBottom = topToBottom;                     // not used by this class, keeps compiler from complaining
  _colorToUse = colorToUse;                       // not used by this class, keeps compiler from complaining
  for (int i=0; i<pixels.numPixels(); i++) {
    _LEDArray[i] = false;
  }
  _active = true;
  _twinkleState = twinkleTurningOn;              // this animation has internal states
  _lastUpdateTime = 0;
  commonTwinkleInitiate();
  Continue();                                    // do first step rather than waiting for an interval
}

/************************************************************************************
 * Continue();  continues randomly lighting LEDs until all are ON
 *              then randomly chooses a few LEDs to turn off and on
 *              Also completes what Finish() starts
 ************************************************************************************/
void Twinkle::Continue() {
  if (!_active) {                               // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();                 // appropriate amount of time elapsed?
  if (int(now - _lastUpdateTime) < _animationStepIncrement) {
    return;                                     // no
  }
  if (_twinkleState == twinkleTwinkling) {      // are we twinkling a few LEDs?
    twinkleSomeLEDs();                          // handle that & return
    return;
  }
 
 // if the animation period has expired (or we've already turned everything on) go to next state
 // but first, make sure all of the LEDs are on. Since they are lit randomly, sometimes not all
 // are ON by the time the animation period expires
 
  if ((_twinkleChangedCount >= (_lastLED-_firstLED)) || (now > _twinkleLongestTimeToWait)) {
    twinkleFinish(_twinkleState == twinkleTurningOn); // either done, or time ran out, either way, finish up now
    if (_twinkleState == twinkleTurningOn) {    // were we turning LEDs on?
      _twinkleState = twinkleTwinkling;         // yes, move to twinkling mode
      twinkleSomeInit();
    } else {
      _active = false;
    }
    return;
  }

// from here down we choose a random LED to light (or turn off); if it's already in the desired state, scan until
// we find a viable candidate LED & use it.

  int tryIdx = random(_firstLED, _lastLED);
  bool desiredState = _twinkleState == twinkleTurningOn;
  uint32_t aColor = randomColor();
  if (_LEDArray[tryIdx] == desiredState) {
    int origTryIdx = tryIdx;
    do {
      tryIdx += 1;
      if (tryIdx > _lastLED) {
        tryIdx = _firstLED;
      }
    } while ((_LEDArray[tryIdx] == desiredState) && (tryIdx != origTryIdx));
  }
  if (_twinkleState == twinkleTurningOff) {
    aColor = offColor;
  }
  _LEDArray[tryIdx] = desiredState;
  _twinkleChangedCount += 1;
  pixels.setPixelColor(tryIdx, aColor);
  pixels.show();
  _lastUpdateTime = now;
}

/************************************************************************************
 * Finish():    intiates randomly turning off random LEDs, Continue()
 *              keeps this process going until all LEDs are OFF
 ************************************************************************************/
void Twinkle::Finish(bool topToBottom) {
  _topToBottom = topToBottom;               // keep compiler quiet
  _twinkleState = twinkleTurningOff;        // set our internal state
  commonTwinkleInitiate();                  // setup for turning off animation
  Continue();                               // and do the first step
}

void Twinkle::printSelf() {
  Serial.print("Twinkle "); Serial.println(_animationStepIncrement);
}

/*!
 * @file ColorSwirl.cpp
 * 
 * @mainpage Arduino library for incrementally wiping a color the length
 * of a NeoPixel strip
 * 
 * @section intro_sec Introduction
 * 
 * This class is derived from Animation and incrementally lights the pixels
 * on a NeoPixel strip. 
 * 
 * The ColorSwirl code was translated from an Aadfruit CircuitPython example
 * minor modifications made to support the Start, Continue, Finish implementation
 * model
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
 * ColorSwirl.cpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */


#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "ColorSwirl.h"
#include "AnimationGlobals.h"

/************************************************************************************
 * Recast of Adafruit CircuitPython function to light the LEDs with a rainbow like
 * selection of colors and then "swirl" those colors
 * minor modifications made to support the Start, Continue, Finish implementation
 * model
 ************************************************************************************/
ColorSwirl::ColorSwirl(const char * animationName, float animationTime, int firstOffset, int lastOffset):Animation(animationName, animationTime, firstOffset, lastOffset) {
}

uint32_t ColorSwirl::wheel(int pos) {
  if ((pos < 0) || (pos > 255)) {
    return offColor;
  }
  if (pos < 85) {
    return pixels.Color(255-pos*3, pos*3, 0);
  }
  if (pos < 170) {
    pos -= 85;
    return pixels.Color(0, 255-pos*3, pos*3);
  }
  pos -= 170;
  return pixels.Color(pos*3, 0, 255-pos*3);
}

// start up the animation
void ColorSwirl::Start(bool topToBottom, uint32_t colorToUse) {
  _topToBottom = topToBottom;                 // not used, compiler happiness
  _colorToUse = colorToUse;                   // ditto
  _active = true;
  _swirlIdx = 0;
  _swirlInc = 1;
  pixels.setBrightness(255);
  for (int i=_firstLED; i<=_lastLED; i++) {
    int rc_index = (i*256/(_lastLED-_firstLED)) + _swirlIdx;
    pixels.setPixelColor(i, wheel(rc_index & 255));
  }
  pixels.show();
  _lastUpdateTime = millis();
}

// keep the animation going until completed
void ColorSwirl::Continue() {
  if (!_active) {                           // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {  // appropriate time elapsed?
    return;                                 // not yet
  }
  _swirlIdx += _swirlInc;
  if (_swirlIdx > 255) {
    _swirlIdx = 254;
    _swirlInc = -_swirlInc;
  }
  for (int i=_firstLED; i<=_lastLED; i++) {
    int rc_index = (i*256/(_lastLED-_firstLED)) + _swirlIdx;
    pixels.setPixelColor(i, wheel(rc_index & 255));
  }
  pixels.show();
  _lastUpdateTime = now;                  // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void ColorSwirl::Finish(bool topToBottom) {
  _topToBottom = topToBottom;               // unused, compiler bliss
  setAllPixelsTo(offColor);
  _active = false;
}

void ColorSwirl::printSelf() {
  Serial.print("Color swirl "); Serial.println(_animationStepIncrement);
}

/************************************************************************************
 * Walk lights like old-time marquee lights
 * Setting marqueeQuanta will override the number of ON LEDs between the OFF LEDs
 ************************************************************************************/
Marquee::Marquee(const char * animationName, float animationTime, int firstOffset, int lastOffset):Animation(animationName, animationTime, firstOffset, lastOffset) {
  if (_animationStepIncrement <= 0) {
    _animationStepIncrement = 150;
  }
  marqueeQuanta = 7;
  int tenPercent = round((_lastLED - _firstLED)/10.0);
  if (marqueeQuanta >  tenPercent) {
    marqueeQuanta = tenPercent+1;
  }
}

void Marquee::Start(bool topToBottom, uint32_t colorToUse) {
  _topToBottom = topToBottom;                 // not used, compiler happiness
  _colorToUse = colorToUse;                   // ditto
  _active = true;
  _marqueeInc = 1;
  if (topToBottom) {
    _marqueeInc = -1;
  }
  _marqueeOffset = 0;
  pixels.setBrightness(mappedBrightness());
  setAllPixelsTo(_colorToUse);
  Continue();
}

// keep the animation going until completed
void Marquee::Continue() {
  if (!_active) {                           // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {  // appropriate time elapsed?
    return;                                 // not yet
  }
// turn on LEDs that were OFF
  for (int i=_firstLED+_marqueeOffset; i<=_lastLED; i = i + marqueeQuanta) {
    pixels.setPixelColor(i, _colorToUse);
  }
  _marqueeOffset += _marqueeInc;
  if (_marqueeOffset < 0) {
    _marqueeOffset = marqueeQuanta-1;
  }
  if (_marqueeOffset >= marqueeQuanta) {
    _marqueeOffset = 0;
  }
// turn OFF next set of LEDs
  for (int i=_firstLED+_marqueeOffset; i<=_lastLED; i = i + marqueeQuanta) {
    pixels.setPixelColor(i, offColor);
  }
  pixels.show();
  _lastUpdateTime = now;                   // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void Marquee::Finish(bool topToBottom) {
  _topToBottom = topToBottom;              // unused, compiler bliss
  setAllPixelsTo(offColor);
  pixels.setBrightness(255);
  pixels.show();
  _active = false;
}

void Marquee::printSelf() {
  Serial.print("Marquee "); Serial.println(_animationStepIncrement);
}

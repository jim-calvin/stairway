/*!
 * @file Animation.cpp
 * 
 * @mainpage Arduino library (base class) for NeoPixel animations
 * 
 * @section intro_sec Introduction
 * 
 * This is a base class for implementing NeoPixel animations that do not
 * use delay().
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
 * Animation.cpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#include "Arduino.h"
#include "Animation.h"
#include <Adafruit_NeoPixel.h>
#include "AnimationGlobals.h"

/************************************************************************************
 * table of colors used by animations and randomColor()
 ************************************************************************************/
#define colorTableSize 15             // # colors in table
uint32_t colors[colorTableSize] = {
  pixels.Color(255,200,100),          // white-ish      0
  pixels.Color(0x00, 0x80, 0x00),     // pale green     1
  pixels.Color(0xFF, 0x00, 0x52),     // rosy           2
  pixels.Color(0x00, 0x00, 0x80),     // light blue     3
  pixels.Color(255, 255, 0),          // yellow         4
  pixels.Color(255, 0, 255),          // magenta        5
  pixels.Color(0, 255, 255),          // cyan           6
  pixels.Color(0x9E, 0x1E, 0x00),     // orange         7
  pixels.Color(0x77, 0x00, 0xA9),     // violet         8
  pixels.Color(0x00, 0x85, 0x82),     // pale cyan      9
  pixels.Color(0xFF, 0x44, 0x44),     // pink-ish      10
  pixels.Color(50, 50, 128),          // dimPaleBlue   11
  pixels.Color(0, 0xDD, 0x15),        // mostly green  12
  pixels.Color(128, 128, 255),        // pale blue     13
  pixels.Color(120, 0, 0)             // redish        14
};

/************************************************************************************
 * pick an random color (and check it's not the same as last time)
 * is referenced via external declaration by all animations
 ************************************************************************************/
uint32_t Animation::randomColor() {
  int newColorIdx = -1;
  do {
    newColorIdx = random(0, colorTableSize);
  } while (newColorIdx == _lastColorIndex);
  _lastColorIndex = newColorIdx;
  return colors[newColorIdx];
}

/************************************************************************************
 * setAllPixelsTo is like fill() except that it only changes the LEDs between, and
 * including _firstLED and _lastLED
 ************************************************************************************/
void Animation::setAllPixelsTo(uint32_t aColor, bool doShow) {
//  pixels.fill(aColor, _firstLED, pixels.numPixels()-2);
  for (int i=_firstLED; i<=_lastLED; i++) {
    pixels.setPixelColor(i, aColor);
  }
  if (doShow) {
    pixels.show();
  }
}

/************************************************************************************
 * Base class for all animations
 * The class has a constructor that set the first & last pixels the animation can
 * use as well as an estimate of time between animation steps based on animationTime
 * and the number of LEDs we can use
 * 
 * The Start() function is used to initiate an animation. A color to use is specified
 * (and may be ignored depending upon the animation) and whether the animation should
 * proceed from top to bottom or vice versa
 * 
 * The Continue() function should be called regularly and at the appropriate time
 * will do the next step of the animation
 * 
 * Finish() is used to initiate the termination of the animation. Finish() may be
 * complete when it returns, but it may just initiate another phase of the animation.
 * 
 * Active() returns true if the animation is executing.
 ************************************************************************************/
Animation::Animation(const char * animationName, float animationTime, int firstOffset, int lastOffset) {
   _animationTime = animationTime;
   if (_animationTime > 0) {      // this calculation assumes time to execute code is negliable
     float waitFraction = animationTime/float(pixels.numPixels()-2);
     _animationStepIncrement = int(round(waitFraction*1000));
   } else {
     _animationStepIncrement = -int(round(_animationTime));
   }
   _firstLED = firstOffset;
   _lastLED = pixels.numPixels()-1+lastOffset;
   _name = animationName;
}

void Animation::Start(bool topToBottom, uint32_t colorToUse) {
  _topToBottom = topToBottom;
  _colorToUse = colorToUse;
  Serial.println("virtual Animation::Start should never be called");
}
    
void Animation::Continue() {
//  Serial.println("virtual Animation::Continue should never be called");  
}
    
void Animation::Finish(bool topToBottom) {
  _topToBottom = topToBottom;
  Serial.println("virtual Animation::Finish should never be called");
}

bool Animation::Active() {
  return _active;
}

void Animation::printSelf() {
  Serial.print(_name); Serial.print(": "); Serial.println(_animationStepIncrement);
}

/************************************************************************************
 * Startup animation is used when we're starting up
 * Simply sequentially lights each LED cycling through the color table as we go
 * may also be used elsewhere
 ************************************************************************************/
Startup::Startup(const char *animationName, float animationTime, int firstOffset, int lastOffset):Animation(animationName, animationTime, firstOffset, lastOffset) { 
  _idx = firstOffset;
  _inc = 1;
  _colorIdx = 0;
}

void Startup::Start(bool topToBottom, uint32_t colorToUse) {
  _topToBottom = topToBottom;                 // not used, compiler happiness
  _colorToUse = colorToUse;                   // non-zero -> cycle through colors[]
  _active = true;
  _idx = _firstLED;
  _inc = 1;
  if (topToBottom) {
    _idx = _lastLED;
    _inc = -1;
  }
    _lastUpdateTime = 0;                      // "schedule" next update
}

// keep the animation going until completed
void Startup::Continue() {
  if (!_active) {                           // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {  // appropriate time elapsed?
    return;                                 // not yet
  }
  uint32_t aColor = _colorToUse;
  if (aColor != 0) {
    aColor = colors[_colorIdx];
  }
  pixels.setPixelColor(_idx, aColor);
  pixels.show();
  _idx += _inc;
  if ((_idx < _firstLED) || (_idx > _lastLED)) {
    _active = false;
  }
  if (aColor != 0) {
    _colorIdx = (_colorIdx+1) % colorTableSize;
  }
  _lastUpdateTime = now;                  // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void Startup::Finish(bool topToBottom) {
  _topToBottom = topToBottom;               // unused, compiler bliss
  _colorToUse = 0;
  _active = true;
  _idx = _firstLED;
  _inc = 1;
  if (topToBottom) {
    _idx = _lastLED;
    _inc = -1;
  }
  _lastUpdateTime = 0;                      // "schedule" next update
}

/*!
 * @file FadeAndWipe.cpp
 * 
 * @mainpage Arduino library for incrementally wiping a color the length
 * of a NeoPixel strip
 * 
 * @section intro_sec Introduction
 * 
 * The ColorWipe class is derived from Animation and incrementally lights 
 * the pixels on a NeoPixel strip. 
 * 
 * FadeToColor lights all LEDs with a specified color, but uses brightness
 * control to fade in, and then out.
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
 * FadeAndWipe.cpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "FadeAndWipe.h"
#include "AnimationGlobals.h"


/************************************************************************************
 * Gradually fades from BLACK to some color
 ************************************************************************************/
FadeToColor::FadeToColor(const char * animationName, float animationTime, int firstOffset, int lastOffset):Animation(animationName, animationTime, firstOffset, lastOffset) {
  _animationStepIncrement = int(round(_animationTime*1000/255));  // different interval computation here
}

// start up the animation
void FadeToColor::Start(bool topToBottom, uint32_t colorToUse) {
  _topToBottom = topToBottom;                   // unused in this animation, keep compiler happy
  _colorToUse = colorToUse;
  _active = true;
  _lastUpdateTime = 0;
  _fadeBrightness = 0;
  _fadeBrightnessInc = 1;
  setAllPixelsTo(_colorToUse, false);           // set LEDs to specified color
  Continue();                                   // do first increment right now
}

// keep the animation going until completed
void FadeToColor::Continue() {
  if (!_active) {                               // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {      // appropriate time elapsed?
    return;                                     // not yet
  }
  pixels.setBrightness(_fadeBrightness);        // set the brightness
  setAllPixelsTo(_colorToUse, false);           // make sure all pixels still correct color
  pixels.show();
  _fadeBrightness += _fadeBrightnessInc;        // step the brightness
  if ((_fadeBrightness > 255) || (_fadeBrightness <= 0)) {
    _active = false;                            // if done, mark that way
  }
  if (!_active) {                               // if just fell out of active
    if (_fadeBrightness <= 0) {                 // and were headed toward off
      _colorToUse = offColor;                   // make the final color black
    }
    setAllPixelsTo(_colorToUse);                // finalize the color
    pixels.setBrightness(255);                  // and return to a good brightness
    pixels.show();
  }
  _lastUpdateTime = now;                        // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void FadeToColor::Finish(bool topToBottom) {
  _topToBottom = topToBottom;                   // unused, keep compiler happy
  _fadeBrightness = 255;                        // you'd think we should switch to offColor, but need original for fade out
  _fadeBrightnessInc = -1;
  _active = true;                               // active again
  Continue();
}

void FadeToColor::printSelf() {
  Serial.print("FadeToColor "); Serial.println(_animationStepIncrement);
}


/************************************************************************************
 * Light LED strip one LED at a time from top to bottom or vice versa
 ************************************************************************************/
ColorWipe::ColorWipe(const char * animationName, float animationTime, int firstOffset, int lastOffset):Animation(animationName, animationTime, firstOffset, lastOffset) {
}

// start up the animation
void ColorWipe::Start(bool topToBottom, uint32_t colorToUse) {
  _topToBottom = topToBottom;
  _colorToUse = colorToUse;
  _active = true;
  _wipeLEDIdx = _firstLED;                      // decide which end of the strip to start from
  _wipeInc = 1;
  if (_topToBottom) {
    _wipeLEDIdx = _lastLED;
    _wipeInc = -1;
  }
  pixels.setBrightness(255);                    // make sure brightness set
  _lastUpdateTime = millis();                   // we've done the first step here
  pixels.setPixelColor(_wipeLEDIdx, _colorToUse);
  pixels.show();
}

// keep the animation going until completed
void ColorWipe::Continue() {
  if (!_active) {                               // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {      // appropriate time elapsed?
    return;                                     // not yet
  }
  _wipeLEDIdx += _wipeInc;
  if ((_wipeLEDIdx > _lastLED) || (_wipeLEDIdx < _firstLED)) {
    _active = false;                            // animation compelete?
    return;
  }
  pixels.setPixelColor(_wipeLEDIdx, _colorToUse); // do next LED
  pixels.show();
  _lastUpdateTime = now;                        // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void ColorWipe::Finish(bool topToBottom) {
  Start(topToBottom, offColor);                 // done exactly the same so re-use Start
}

void ColorWipe::printSelf() {
  Serial.print("ColorWipe "); Serial.println(_animationStepIncrement);
}

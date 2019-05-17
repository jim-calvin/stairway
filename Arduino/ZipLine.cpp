/*!
 * @file ZipLine.cpp
 * 
 * @mainpage Arduino library for lighting an animating an individual LED the length
 * of a NeoPixel strip. Derived classes do something similar.
 * 
 * @section intro_sec Introduction
 * 
 * ZipLine is derived from Animation and lights a single LED moving it
 * from one end of the strip to the other and vice-versa.
 * 
 * ZipLineInverse lights the strip with a specified color and then
 * moves a black LED back and forth across the strip.
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
 * ZipLine.cpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */


#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "ZipLine.h"
#include "AnimationGlobals.h"

/************************************************************************************
 * rapidly move a single pixel back and forth
 ************************************************************************************/
ZipLine::ZipLine(const char * animationName, float animationTime, int firstOffset, int lastOffset):Animation(animationName, animationTime, firstOffset, lastOffset) {
}

// start up the animation
void ZipLine::Start(bool topToBottom, uint32_t colorToUse) {
  _topToBottom = topToBottom;
  _colorToUse = colorToUse;
  _active = true;
  _zipIdx = _firstLED;
  _zipInc = 1;
  if (_topToBottom) {                         // assume top to bottom to start
    _zipIdx= _lastLED;                        // but caller wanted bottom to top
    _zipInc = -1;
  }
  pixels.setBrightness(mappedBrightness());   // use a dim version of whatever color
  setAllPixelsTo(offColor, false);
  _lastUpdateTime = millis();
  pixels.setPixelColor(_zipIdx, _colorToUse); // moving a single pixel back and forth
  pixels.show();
  _lastUpdateTime = millis();                 // we've done first step here
}

// keep the animation going until completed
void ZipLine::Continue() {
  if (!_active) {                             // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {    // appropriate time elapsed?
    return;                     // not yet
  }
  pixels.setPixelColor(_zipIdx, offColor);    // turn off currently on LED
  _zipIdx += _zipInc;                         // step to next
  if (_zipIdx > _lastLED) {                   // off end?
    _zipIdx = _lastLED-1;                     // reverse direction
    _zipInc = -_zipInc;
  } else if (_zipIdx < _firstLED) {           // off other end
    _zipIdx = _firstLED+1;                    // yep
    _zipInc = -_zipInc;
  }
  pixels.setPixelColor(_zipIdx, _colorToUse);  // light new pixel
  pixels.show();
  _lastUpdateTime = now;                       // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void ZipLine::Finish(bool topToBottom) {
  _topToBottom = topToBottom;
  _active = false;                             // simple, just set pixels black, restore brightness & stop
  setAllPixelsTo(offColor);
  pixels.setBrightness(255);
  pixels.show();
}


/************************************************************************************
 * light all LEDs and the rapidly move a single dark pixel back and forth
 ************************************************************************************/
ZipLineInverse::ZipLineInverse(const char * animationName, float animationTime, int firstOffset, int lastOffset):ZipLine(animationName, animationTime, firstOffset, lastOffset) {
}

// start up the animation; nearly identical to ZipLine
void ZipLineInverse::Start(bool topToBottom, uint32_t colorToUse) {
  ZipLine::Start(topToBottom, colorToUse);
  setAllPixelsTo(colorToUse);                // but here we light the strip and move a dark pixel
}

// keep the animation going until completed
void ZipLineInverse::Continue() {
  if (!_active) {                            // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {   // appropriate time elapsed?
    return;                                  // not yet
  }
  pixels.setPixelColor(_zipIdx, _colorToUse); // turn the pixel back on
  _zipIdx += _zipInc;
  if (_zipIdx > _lastLED) {                 // off end?
    _zipIdx = _lastLED-1;                   // reverse direction
    _zipInc = -_zipInc;
  } else if (_zipIdx < _firstLED) {         // other end?
    _zipIdx = _firstLED+1;                  // yes, reverse
    _zipInc = -_zipInc;
  }
  pixels.setPixelColor(_zipIdx, offColor);  // turn off new pixel
  pixels.show();
  _lastUpdateTime = now;                    // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void ZipLineInverse::Finish(bool topToBottom) {
  ZipLine::Finish(topToBottom);                 // use ZipLine's finish
}

/************************************************************************************
 * Like Zip, but starts a pixel and each end of the neopixel strip
 ************************************************************************************/
Zip2::Zip2(const char * animationName, float animationTime, int firstOffset, int lastOffset):ZipLine(animationName, animationTime, firstOffset, lastOffset) {
}

// start up the animation
void Zip2::Start(bool topToBottom, uint32_t colorToUse) {
  ZipLine::Start(topToBottom, colorToUse);
  _zip2Idx = _lastLED;
   if (topToBottom) {
    _zip2Idx = _firstLED;
  }
  pixels.setPixelColor(_zipIdx, _colorToUse); // moving a single pixel back and forth
  pixels.setPixelColor(_zip2Idx, _colorToUse);
  pixels.show();
}

// keep the animation going until completed
void Zip2::Continue() {
  if (!_active) {                             // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {    // appropriate time elapsed?
    return;                     // not yet
  }
  pixels.setPixelColor(_zipIdx, offColor);    // turn off currently on LED
  pixels.setPixelColor(_zip2Idx, offColor);
  _zipIdx += _zipInc;                         // step to next
  if (_zipIdx > _lastLED) {                   // off end?
    _zipIdx = _lastLED-1;                     // reverse direction
    _zipInc = -_zipInc;
  } else if (_zipIdx < _firstLED) {           // off other end
    _zipIdx = _firstLED+1;                    // yep
    _zipInc = -_zipInc;
  }
  _zip2Idx -= _zipInc;
  if (_zip2Idx > _lastLED) {                   // off end?
    _zip2Idx = _lastLED-1;                     // reverse direction
  } else if (_zip2Idx < _firstLED) {           // off other end
    _zip2Idx = _firstLED+1;                    // yep
  }
  pixels.setPixelColor(_zipIdx, _colorToUse);  // light new pixel
  pixels.setPixelColor(_zip2Idx, _colorToUse);  // light new pixel
  pixels.show();
  _lastUpdateTime = now;                       // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void Zip2::Finish(bool topToBottom) {
  ZipLine::Finish(topToBottom);
}


/************************************************************************************
 * Like ZipInverse, but starts a dark pixel and each end of the neopixel strip
 ************************************************************************************/
Zip2Inverse::Zip2Inverse(const char * animationName, float animationTime, int firstOffset, int lastOffset):Zip2(animationName, animationTime, firstOffset, lastOffset) {
}

// start up the animation; nearly identical to ZipLine
void Zip2Inverse::Start(bool topToBottom, uint32_t colorToUse) {
  ZipLine::Start(topToBottom, colorToUse);
  _zip2Idx = _lastLED;
  if (topToBottom) {
    _zip2Idx = _firstLED;
  }
  setAllPixelsTo(_colorToUse, false);
  pixels.setPixelColor(_zipIdx, offColor); // moving a single pixel back and forth
  pixels.setPixelColor(_zip2Idx, offColor);
  pixels.show();
}

// keep the animation going until completed
void Zip2Inverse::Continue() {
  if (!_active) {                            // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) {   // appropriate time elapsed?
    return;                                  // not yet
  }
  pixels.setPixelColor(_zipIdx, _colorToUse); // turn the pixel back on
  pixels.setPixelColor(_zip2Idx, _colorToUse); // turn the pixel back on
  _zipIdx += _zipInc;
  if (_zipIdx > _lastLED) {                 // off end?
    _zipIdx = _lastLED-1;                   // reverse direction
    _zipInc = -_zipInc;
  } else if (_zipIdx < _firstLED) {         // other end?
    _zipIdx = _firstLED+1;                  // yes, reverse
    _zipInc = -_zipInc;
  }
  _zip2Idx -= _zipInc;
  if (_zip2Idx > _lastLED) {                // off end?
    _zip2Idx = _lastLED-1;                  // reverse direction
  } else if (_zip2Idx < _firstLED) {        // off other end
    _zip2Idx = _firstLED+1;                 // yep
  }
  pixels.setPixelColor(_zipIdx, offColor);  // turn off new pixel
  pixels.setPixelColor(_zip2Idx, offColor); // turn off new pixel
  pixels.show();
  _lastUpdateTime = now;                    // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void Zip2Inverse::Finish(bool topToBottom) {
  ZipLine::Finish(topToBottom);             // use ZipLine's finish
}


/************************************************************************************
 * Lke Zip, but chooses a random color every so many steps
 ************************************************************************************/
ZipR::ZipR(const char * animationName, float animationTime, int firstOffset, int lastOffset):ZipLine(animationName, animationTime, firstOffset, lastOffset) {
}

// start up the animation
void ZipR::Start(bool topToBottom, uint32_t colorToUse) {
  ZipLine::Start(topToBottom, colorToUse);
  repeatCount = 0;
}

// keep the animation going until completed
void ZipR::Continue() {
  if (!_active) {                           // nothing to do here if we're not active
    return;
  }
  unsigned long now = millis();
  int elapsed = (now - _lastUpdateTime);
  if (elapsed < _animationStepIncrement) { // appropriate time elapsed?
    return;                     // not yet
  }
  pixels.setPixelColor(_zipIdx, offColor);    // turn off currently on LED
  _zipIdx += _zipInc;                         // step to next
  if (_zipIdx > _lastLED) {                   // off end?
    _zipIdx = _lastLED-1;                     // reverse direction
    _zipInc = -_zipInc;
  } else if (_zipIdx < _firstLED) {           // off other end
    _zipIdx = _firstLED+1;                    // yep
    _zipInc = -_zipInc;
  }
  repeatCount = (repeatCount + 1) % 4;
  if (repeatCount == 0) {
    _colorToUse = randomColor();
  }
  pixels.setPixelColor(_zipIdx, _colorToUse);  // light new pixel
  pixels.show();
  _lastUpdateTime = now;                       // "schedule" next update
}

// start the shutdown of the animation; in this case it's just running it again but with color=black
void ZipR::Finish(bool topToBottom) {
  ZipLine::Finish(topToBottom);
}

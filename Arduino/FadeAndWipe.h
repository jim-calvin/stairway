/*!
 * @file FadeAndWipe.h
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
 * FadeAndWipe.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */
 
#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "Animation.h"

#ifndef FadeAndWipe_h
#define FadeAndWipe_h


/************************************************************************************
 * Gradually fades from BLACK to some color
 ************************************************************************************/
class FadeToColor : public Animation {
  public:
    FadeToColor(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();
  
  private:
    int _wipeLEDIdx;
    int _wipeInc;
    int _fadeBrightness;
    int _fadeBrightnessInc;
};


/************************************************************************************
 * Light LED strip one LED at a time from top to bottom or vice versa
 ************************************************************************************/
class ColorWipe : public Animation {
  public:
    ColorWipe(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();

  private:
    int _wipeLEDIdx;
    int _wipeInc;
};

#endif

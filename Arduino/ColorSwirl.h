/*!
 * @file ColorSwirl.h
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
 * ColorSwirl.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */
#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "Animation.h"

#ifndef ColorSwirl_h
#define ColorSwirl_h

/************************************************************************************
 * Recast of Adafruit CircuitPython function to light the LEDs with a rainbow like
 * selection of colors and then "swirl" those colors
 ************************************************************************************/
class ColorSwirl : public Animation {
  public:
    ColorSwirl(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();
  protected:
    int _swirlIdx;
    int _swirlInc;

    uint32_t wheel(int pos);
};

/************************************************************************************
 * All LEDs one color, but cycle through the rainbow
 ************************************************************************************/
class SingleSwirl : public ColorSwirl {
  public:
    SingleSwirl(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();
};

/************************************************************************************
 * Walk lights like old-time marquee lights
 * Setting marqueeQuanta will override the number of ON LEDs between the OFF LEDs
 ************************************************************************************/
class Marquee : public Animation {
  public:
    Marquee(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();
 
    int marqueeQuanta;             // # of ON LEDs between OFF LEDs
 
  private:
    int _marqueeOffset;
    int _marqueeInc;
};

#endif

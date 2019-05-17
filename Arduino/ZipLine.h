/*!
 * @file ZipLine.h
 * 
 * @mainpage Arduino library for incrementally wiping a color the length
 * of a NeoPixel strip
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
 * ZipLine.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */
#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "Animation.h"

#ifndef ZipLine_h
#define ZipLine_h

/************************************************************************************
 * rapidly move a single pixel back and forth
 ************************************************************************************/
class ZipLine : public Animation {
  public:
    ZipLine(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();

  protected:
    int _zipIdx;
    int _zipInc;
};

/************************************************************************************
 * light all LEDs and the rapidly move a single dark pixel back and forth
 ************************************************************************************/
class ZipLineInverse : public ZipLine {
    public:
    ZipLineInverse(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();

};

/************************************************************************************
 * Like Zip, but starts a pixel and each end of the neopixel strip
 ************************************************************************************/
class Zip2 : public ZipLine {
  public:
    Zip2(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();

  protected:
    int _zip2Idx;
};

/************************************************************************************
 * Like ZipInverse, but starts a dark pixel and each end of the neopixel strip
 ************************************************************************************/
class Zip2Inverse : public Zip2 {
  public:
    Zip2Inverse(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();
};

/************************************************************************************
 * Lke Zip, but chooses a random color every so many steps
 ************************************************************************************/
class ZipR : public ZipLine {
  public:
    ZipR(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();

  protected:
    int repeatCount;
};

#endif

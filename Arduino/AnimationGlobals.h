/*!
 * @file AnimationGlobals.h
 * 
 * @mainpage Arduino library (base class) for NeoPixel animations
 * 
 * @section intro_sec Introduction
 * 
 * This header file defines data which is constant across the base
 * Animations and any derived classes. The items defined here must
 * be instantiated and their values initialized. As supplied, the
 * main module "stairway" does this.
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
 * AnimationGlobals.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#ifndef Animation_Globals_H
#define Animation_Globals_H

// There's probably a better way of doing all of this, I just haven't figured
// it out yet.

#define kNumberOfLEDs 111                 // # of LEDs in our NeoPixel strip

// the following definitions _must_ exist elsewhere in the project
// as shipped, they are defined in the stairway file.

extern uint32_t indicatorColor;           // color to use for PIR indicators
extern uint32_t offColor;                 // offColor (black)
extern Adafruit_NeoPixel pixels;          // the strip of NeoPixels

extern uint32_t randomColor();            // returns a color from colors[]
extern int mappedBrightness(); // returns a brightness level to use

#endif

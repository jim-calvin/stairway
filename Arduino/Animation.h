/*!
 * @file Animation.h
 * 
 * @mainpage Arduino library (base class) for NeoPixel animations
 * 
 * @section intro_sec Introduction
 * 
 * This is a base class for implementing NeoPixel animations that do not
 * use delay().
 * 
 * Animations are devided into 4 parts:
 *  1. The constructor. Among other things, the constructor calculates
 *     a time value (in milliseconds) to wait between steps of the
 *     animation.
 *     This class (Animation) should be called by all derived class
 *     constructors.
 *  2. Start. This function (re)initializes any variables required for
 *     the animation. It may call Continue() to get the animation
 *     rolling.
 *  3. Continue. This function should be called from the main loop.
 *     The function should use the _animationStepIncrement and
 *     _lastUpdateTime variables to decide when the next step of the
 *     animation should occur.
 *  4. Finish. This function is used to wrap up the animation. It may
 *     be as simple as deactivating (_active=false) and blanking the
 *     pixel display. Or, it may initiate another animation sequence
 *     that eventually finishes by deactivating.
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
 * Animation.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>

#ifndef Animation_h
#define Animation_h

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
 class Animation {
  public:
    Animation(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);         // <0 is exact value to use for _animationStepIncrement
    virtual void Start(bool topToBottom, uint32_t colorToUse);  // Initate animation
    virtual void Continue() = 0;          // keep animation going
    virtual void Finish(bool topToBottom);  // initiate completion of animation
    bool Active();                        // is the animation currently active?
    void printSelf();                     // print animation name and _animationStepIncrement

    uint32_t randomColor();               // returns a random color from colors[] table (see .cpp file)

  protected:
    const char * _name = "Base class";    // name of animation
    int _firstLED;                        // address of 1st pixel in strip we can use
    int _lastLED;                         // address of last pixel in strip we can use
    bool _active;                         // true if animation is currently displaying something
    uint32_t _colorToUse;                 // suggested color to use
    bool _topToBottom;                    // animation clue, start animation from top
    unsigned long _lastUpdateTime;        // last time (millis) that animation was updated
    float _animationTime;                 // amount of time (float seconds) the animation should take
    int _animationStepIncrement;          // time (millis) between updates
    int _lastColorIndex;                  // used by randomColor()
 
    void setAllPixelsTo(uint32_t aColor, bool doShow=true);   // like .fill() except that goes between first & last
};

/************************************************************************************
 * animation that incrementally lights LEDs in the strip. Cycles through the colors
 * table for colors to light the LEDs
 ************************************************************************************/
class Startup : public Animation {
  public:
    Startup(const char * animationName, float animationTime, int firstOffset=1, int lastOffset=-1);
    void Start(bool topToBottom, uint32_t colorToUse);
    void Continue();
    void Finish(bool topToBottom);
    bool Active();
    void printSelf();
 
  private:
    int _idx;
    int _inc;
    int _colorIdx;
};
  

#endif

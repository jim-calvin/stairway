/*!
 * @file starway.ino
 * 
 * @mainpage Main for stairway lighting project
 * 
 * @section intro_sec Introduction
 * 
 * This file contains the startup() and loop() functions for the stairway 
 * lighting project (as well as some support functions - lightlevel,
 * state machine & associated functions, color and animation selection
 * functions)
 * 
 * @section author Author
 * 
 * Written by Jim Calvin.
 * 
 * @section dependencies Dependencies
 * 
 * This file depends on multiple Adafruit library and board definitions
 * 
 * This project also requires the following files:
 * Animation.cpp/.h, AnimationGlobals.h, ColorSwirl.cpp/.h,
 * FadeAndWipe.cpp/.h, PIR.cpp/.h, Twinkle.cpp/.h,
 * ZipLine.cpp/.h
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
 * stairway.ino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 */

#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#include "PIR.h"
#include "Animation.h"
#include "FadeAndWipe.h"
#include "Twinkle.h"
#include "ZipLine.h"
#include "ColorSwirl.h"
#include "AnimationGlobals.h"         // defines # of LEDs in the string (among other things)

bool debug = false;                   // set true for debugging output on Serial monitor
bool debugPattern = false;            // set true when debugging animations, eliminates wait for PIRs==LOW
bool PIRDebug = false;                // set true when using switches for PIRs
bool fadeDebug = false;               // true when no mode switch attached
bool lightLevelDebug = false;         // when we don't have a light sensor connected

/************************************************************************************
 * GPIO pin definitions. We're using all of them on a trinket M0
 ************************************************************************************/
#define TopPIRPin     0               // where upper PIR sensor output is found
#define NeoPixelsPin  1               // pin neopixels are attached to
#define modePin       2               // our display mode setting
#define BottomPIRPin  3               // lower PIR
#define LightLevelPin 4               // for sensing the light level (analog in)

/************************************************************************************
 * light levels to used to decide which color to light the stairs
 ************************************************************************************/
#define klightLevelBright 875
int lightLevelBright = klightLevelBright;  // above this, brightish white
#define klightLevelMedium 500
int lightLevelMedium = klightLevelMedium;  // below this, a dimmer pale blue
#define klightLevelDim 225
int lightLevelDim = klightLevelDim;   // below this, a dimish red
#define klightLevelTwinkleThreshold 600
int lightLevelTwinkleThreshold = klightLevelTwinkleThreshold;  // separator between twinkle mode & fade mode

#define minimumOnTime 10              // amount of time (seconds) LEDs stay on after 2nd PIR is triggered
#define timeBetweenPIRTriggers 9      // time out period (seconds) when waiting for 2nd PIR to trip

/************************************************************************************
 * lots of LEDs to light stairs.
 * use the first and last of the strip as indicators for the PIRs
 ************************************************************************************/
const int numberOfPixels = kNumberOfLEDs; // defined in AnimationGlobals

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numberOfPixels, NeoPixelsPin, NEO_GRB + NEO_KHZ800);
Adafruit_DotStar dot = Adafruit_DotStar(1, INTERNAL_DS_DATA, INTERNAL_DS_CLK, DOTSTAR_BGR);

PIR topPIR = PIR(TopPIRPin, numberOfPixels-1, "top", PIRDebug);
PIR bottomPIR = PIR(BottomPIRPin, 0, "bottom", PIRDebug);

// define some named colors for use in the code
uint32_t onColor = pixels.Color(255, 255, 255);      // color to use for ON
uint32_t offColor = pixels.Color(0, 0, 0);           // color to use for OFF
uint32_t indicatorColor = pixels.Color(10, 10, 10);  // color to use for indicator LEDs
uint32_t dimRed = pixels.Color(75, 0, 0);
uint32_t dimPaleBlue = pixels.Color(50, 50, 128);

// below are used to attempt to normalize readings from the light sensors
int lastLevel = 0;                // last actual light level reading (use when LEDs on)
int lightLevelMin = 1024;         // lowest light level seend during some period
int lightLevelMax = 0;            // highest light level seen during some period
unsigned long LightLevelSamplingStartTime = 0;

/************************************************************************************
 * attempt to rescale the light thresholds based on readings seen over some time period
 ************************************************************************************/
void remapLightLevelThresholds() {
    if (debug) { Serial.println("remapLightLevelThresholds"); }
    lightLevelBright = map(klightLevelBright, 0, 1024, lightLevelMin, lightLevelMax);
    lightLevelMedium = map(klightLevelMedium, 0, 1024, lightLevelMin, lightLevelMax);
    lightLevelDim = map(klightLevelDim, 0, 1024, lightLevelMin, lightLevelMax);
    lightLevelTwinkleThreshold = map(klightLevelTwinkleThreshold, 0, 1024, lightLevelMin, lightLevelMax);
}

/************************************************************************************
 * determine if mode switch is set or not
 ************************************************************************************/
bool fadingMode() {
  if (fadeDebug) {
    bool debugResult = (random(4873823) & 1) == 0;
    Serial.print("debugFadeMode: "); Serial.println(debugResult);
    return debugResult;
  }
  return (digitalRead(modePin) == LOW);
}

/************************************************************************************
 * Use the onboard dotstar as an indicator; turns off the inidicator after ~30 seconds
 ************************************************************************************/
unsigned long inidicatorLastTime = 0;

bool indicatorOn = false;
void setIndicator(int r, int g, int b) {
  dot.setPixelColor(0, r, g, b);
  dot.show();
  if ((r != 0) || (g !=0) || (b != 0)) {  // turned it on?
    inidicatorLastTime = millis();        // yes, setup to turn off in a while
    indicatorOn = true;
  }
}

bool indicatorContinue() {
  if (!indicatorOn) {                     // indicator on?
    return false;                         // no
  }
  if ((millis()-inidicatorLastTime) < (30*1000)) {
    return true;                          // on, but not ready to turn OFF
  }
  indicatorOn = false;                    // turn it off
  dot.setPixelColor(0, 0, 0, 0);
  dot.show();
  return false;
}

/************************************************************************************
 * blink the status LED on the processor to show we're active
 ************************************************************************************/
unsigned long lastTime = millis();
int activeState = HIGH;
void blinkActive(unsigned long now, bool useDotStar=false) {
  if ((now - lastTime) >= 500) {          // blink RED LED to show we're active
    if (useDotStar) {                     // used occasionally (like at start up)
      if (activeState == HIGH) {
        setIndicator(0x7E, 0x1E, 0x00);
      } else {
        setIndicator(0, 0, 0);
      }
    } else {
      digitalWrite(13, activeState);
    }
    if (activeState == HIGH) {
      activeState = LOW;
    } else {
      activeState = HIGH;
    }
    lastTime = now;
  }
}

/************************************************************
 * simple state machine for implementation
 ***********************************************************/
enum SystemState { idleState, topTriggeredState, bottomTriggeredState, waitingTimeoutState, failedTimeoutState };

SystemState currentState = idleState;

/************************************************************
 *  create objects for the various animations
 ***********************************************************/
ColorWipe colorWipe = ColorWipe("ColorWipe", 1.3);
ZipLine zipLine = ZipLine("ZipLine", 2.0);
ColorSwirl colorSwirl = ColorSwirl("Rainbow", -15);
SingleSwirl singleSwirl = SingleSwirl("Fading colors", -15);
Marquee marquee = Marquee("Marquee", -150);
FadeToColor fadeToColor = FadeToColor("Fade to color", 1.3);
ZipLineInverse zipLineInverse = ZipLineInverse("ZipLine Inverse", 2.0);
Twinkle twinkle = Twinkle("Twinkle", 1.75);
Startup sup = Startup("Startup", 1.5);
Zip2 zip2 = Zip2("Zip2", 2.0);
Zip2Inverse zip2i = Zip2Inverse("Zip 2 inverse", 2.0);
ZipR zipR = ZipR("Zip Random", 1.75);

// animations to use when mode switch is LOW
#define nonFadeDimAnimationCount 2   // number used in LOW mode
Animation * nonFadeDimAnimations[nonFadeDimAnimationCount] = {
  &colorWipe,
  &sup
};

#define nonFadeBrighterAnimationCount 3   // number used in LOW mode
Animation * nonFadeBrighterAnimations[nonFadeBrighterAnimationCount] = {
  &zipLine,
  &zip2,
  &zipR
};

// animations to use when mode switch is HIGH
#define fadeHighAnimationCount 9  // number used in HI mode
Animation * fadeHighAnimations[fadeHighAnimationCount] = {
  &singleSwirl,
  &twinkle,
  &colorSwirl,
  &marquee,
  &twinkle,
  &fadeToColor,
  &zipLineInverse,
  &colorWipe,
  &zip2i
 };

// currently active animation - initialize to something
Animation *currentAnimation = fadeHighAnimations[fadeHighAnimationCount-1];

// display to show at startup - displays all of entries in colors[]
void startupShowColors() {
  bool topToBottom = false;
  for (int i=0; i<4; i++) {     // repeat a few times
    unsigned long started = millis();
    unsigned long now = started;
    sup.Start(topToBottom, 1);
    do {
      now = millis();
      sup.Continue();           // continue & wait
      blinkActive(now);
    } while ((now-started) < 2500);
    started = now;
    sup.Finish(topToBottom);   // finish then wait a bit
    do {
      now = millis();
      sup.Continue();
      blinkActive(now);
    } while ((now-started) < 2500);
    topToBottom = !topToBottom;
  }
}

/************************************************************************************
 * gets the current light level; except:
 *   if the LEDs are on, it returns previous reading from when they weren't on
 *   if the level hasn't changed very much, hands back previous reading
 ************************************************************************************/
int getLightLevel() {
  if (lightLevelDebug) {
    int debugResult = random(986342) & 1 ? lightLevelBright : lightLevelDim;
    Serial.print("debugLightLevel: "); Serial.println(debugResult);
    return debugResult;
  }
  if (currentAnimation->Active()) {
    if (debug) { Serial.println("currentAnimation->Active() == true"); }
    return lastLevel;             // LEDS being will likely foul the reading, just return last one
  }
  int level = analogRead(LightLevelPin);
  if (level > lightLevelMax) {    // track max and min values we've seen over some period
    lightLevelMax = level;
  }
  if (level < lightLevelMin) {
    lightLevelMin = level;
  }
  if (abs(level-lastLevel) > 3) { // did it change much?
    if (debug) { Serial.print("light level: "); Serial.println(level); }
    lastLevel = level;
  }
  return level;
}

/************************************************************************************
 * myRandom
 * return a pseudo random number in a range
 * aRandomNumber is updated every cycle of the loop() function
 ************************************************************************************/
long aRandomNumber = 0;

int myRandom(int modulo) {
  return aRandomNumber % modulo;  
}

uint32_t colorBasedOnConditions() {
  uint32_t theColor = currentAnimation->randomColor();
  if (!fadingMode()) {         // mode pin in fade mode?
    if (currentAnimation == &colorWipe) {
      if (debug) { Serial.println("Fading mode && ColorWipe"); }
      int lightLevel = getLightLevel();
      if (lightLevel > lightLevelBright) {
        theColor = offColor;
      }
      if (lightLevel < lightLevelMedium) {
        theColor = dimPaleBlue;
      }
      if (lightLevel < lightLevelDim) {
        theColor = dimRed;
      }
    }
  }
  return theColor;
}

/************************************************************************************
 * choose an animation based on fade mode switch
 * also, don't do same animation twice in a row
 ************************************************************************************/
int animationIndex = 0;
int previousAnimationIndex = 0;
int animationFadeIndex = 0;
int previousAnimationFadeIndex = 0;
void chooseAnimation() {
  if (fadingMode()) {
    animationFadeIndex = myRandom(fadeHighAnimationCount);
    if (animationFadeIndex == previousAnimationFadeIndex) {
      animationFadeIndex = (animationFadeIndex+1) % fadeHighAnimationCount;
    }
    animationFadeIndex = animationFadeIndex % fadeHighAnimationCount;
    currentAnimation = fadeHighAnimations[animationFadeIndex];
    previousAnimationFadeIndex = animationFadeIndex;
  } else {    // in this mode, we take light level into account choosing an animation
    int lvl = getLightLevel();
    if (lvl > lightLevelMedium) {     // somewhat bright?
      animationIndex = myRandom(nonFadeBrighterAnimationCount);
      if (animationIndex == previousAnimationIndex) {
        animationIndex = (animationIndex+1) % nonFadeBrighterAnimationCount;
      }
      currentAnimation = nonFadeBrighterAnimations[animationIndex];
    } else {                          // less bright
      animationIndex = myRandom(nonFadeDimAnimationCount);
      if (animationIndex == previousAnimationIndex) {
        animationIndex = (animationIndex+1) % nonFadeDimAnimationCount;
      }
      currentAnimation = nonFadeDimAnimations[animationIndex];
    }
    previousAnimationIndex = animationIndex;
  }
//  currentAnimation->printSelf();
}

// an attempt to normalize readings from the photoresistor
unsigned long lastLightReadTime = 0;
bool firstDay = true;

void processLightLevel(unsigned long now) {
  if (firstDay && (now-LightLevelSamplingStartTime) > 1000*60*30) {
    remapLightLevelThresholds();
    LightLevelSamplingStartTime = now;
  }
  if ((now-LightLevelSamplingStartTime) > 1000*60*60*24) {
    remapLightLevelThresholds();
    LightLevelSamplingStartTime = now;
    lightLevelMin = 1024;
    lightLevelMax = 0;
    firstDay = false;
  }
  if ((now-lastLightReadTime) > 1000*10) {
    getLightLevel();         // to build round the clock samples
    lastLightReadTime = now;
  }
}

// externally used function to map current light level to an appropriate brightness
int mappedBrightness() {
  int brightness = map(getLightLevel(), lightLevelMin, lightLevelMax, 64, 255);
  if (debug) { 
    Serial.print("mappedBrightness "); Serial.print(getLightLevel());
    Serial.print(" -> "); Serial.println(brightness);
  }
  return brightness;
}


/************************************************************************************
 * Standard setup function
 * initializes Serial, pixels, dotstar
 * does a display on the LED strip
 * and then waits for the PIRs to both got to idle state
 ************************************************************************************/
void setup() {
  Serial.begin(115200);         // setup serial
  pixels.begin();               // setup the pixel strip
  pixels.show();

  pinMode(modePin, INPUT_PULLUP);

  for (int i=0; i<10; i++) {    // wait a bit for serial to sych up
    Serial.print(".");
    delay(250);
  }
  Serial.println("");
  Serial.print("Starting with "); Serial.print(fadeHighAnimationCount); Serial.print(" fade animations, ");
  Serial.print(nonFadeDimAnimationCount); Serial.print(" dim & "); Serial.print(nonFadeBrighterAnimationCount); 
  Serial.print(" brighter non-fade animations, and ");
  Serial.print(pixels.numPixels()); Serial.println(" LEDs");

// if set true, print out debugging states so we know what we're running
  if (topPIR.debugMode()) { Serial.println("  >> Main: PIRDebug == true"); }
  if (debug) { Serial.println("  >> Main: debug == true"); }
  if (debugPattern) { Serial.println("  >> Main: debugPattern == true"); }
  if (fadeDebug) { Serial.println("  >> Main: fadeDebug == true"); }
  if (lightLevelDebug) {Serial.println("  >> Main: lightLevelDebug == true"); }

  pixels.fill(offColor);        // blank neopixel display

  bool top, bottom;
  bool state = false;
  top = topPIR.read();          // prime the pump so first real call is accurate
  bottom = bottomPIR.read();

  startupShowColors();
  randomSeed(analogRead(4));
  if (top || bottom) {          // either PIR in active state?
    Serial.print("Waiting for PIRs to stablize, top: "); Serial.print(top); Serial.print(" bottom: "); Serial.println(bottom);
    if (!debugPattern) {
      do {                        // check the PIR states until they are both "false" (stabilized)
        delay(250);
        if (state) {              // blink dotstar while waiting
          dot.setPixelColor(0, 0, 0, 0);
        } else {
          dot.setPixelColor(0, 128, 96, 0);
        }
        dot.show();
        state = !state;
        top = topPIR.read();
        bottom = bottomPIR.read();
      } while (top || bottom);
      setIndicator(0, 0, 0);
    } else {
      Serial.println("  Started in debugPattern mode");
      setIndicator(64, 92, 64);
      zipLine.Start(true, zipLine.randomColor()); // start pattern we're debugging
    }
  }
  Serial.println("Ready");
}

/************************************************************************************
 * Helper functions for the state machine execution
 * handles code essentially duplicated otherwise
 * hopefully makes
 ************************************************************************************/
unsigned long onTime = 0;     // remember time we turned leds on
bool startedAtTop = false;    // remember if the PIR at the top tripped first (used rather than expanding the state machine)
bool prevTop = false;
bool prevBot = false;

void firstSensorTripped(int nextState, bool fromTop, uint32_t now, int iRed, int iBlue, int iGreen) {
  chooseAnimation();                        // select an animation to use
  setIndicator(iRed, iBlue, iGreen);        // set indicator so state can be observed
  startedAtTop = fromTop;                   // how pattern started
  onTime = now;                             // when it started
  if (debug) { currentAnimation->printSelf(); }
  currentAnimation->Start(fromTop, colorBasedOnConditions()); // and start animation going
  currentState = SystemState(nextState);    // this (and the int parameter) are a hack to avoid an
                                            // arduino compile time problem
}

void secondSensorTripped(uint32_t now, int iRed, int iBlue, int iGreen) {
  setIndicator(iRed, iGreen, iBlue);        // observable state info
  onTime = now;
  currentState = waitingTimeoutState;       // next state
}

void noSecondSensor(bool fromTop, int iRed, int iGreen, int iBlue) {
  setIndicator(iRed, iGreen, iBlue);        // observable state info
  currentAnimation->Finish(fromTop);        // start the animation in Finish phase
  currentState = failedTimeoutState;        // and next state
}

/************************************************************************************
 * state machine execution - uses Trinket M0 dotstar to display current state
 * 
 * DotStar color indicators for debugging
 * RED - top PIR tripped
 * GREEN - bottom PIR tripped
 * BLUE - top then bottom PIRs tripped
 * PALE BLUE - top tripped, timed out waiting for bottom PIR
 * MAGENTA - bottom then top PIRs tripped
 * YELLOW - bottom tripped, timed out waiting for top PIR
 * BLACK - idle state (other state colors are set BLACK after 30 seconds)
 ************************************************************************************/
void executeStateMachine(bool top, bool bottom, uint32_t now) {
   switch (currentState) {
    case idleState:                       // waiting for a PIR to sense motion
      if (top) {
        if (debug) { Serial.println("-> top triggered"); }
        firstSensorTripped(topTriggeredState, true, now, 128, 0, 0);
      } else if (bottom) {
        if (debug) { Serial.println("-> bottom Triggered"); }
        firstSensorTripped(bottomTriggeredState, false, now, 0, 128, 0);
      }
      break;
    case topTriggeredState:               // top saw motion, wait for timeout, or bottom PIR
      if (bottom) {                       // now bottom saw motion
        if (debug) { Serial.println("-> waitingTimeout top"); }
        secondSensorTripped(now, 0, 0, 128);
      } else {                            // lower PIR not triggered yet; timeout yet?
        if (((now - onTime)/1000 >= timeBetweenPIRTriggers) /*|| !top*/) {  // maybe comment out "|| !top"
                                                                        // which forces lights to stay on for time
                                                                        // period rather than when the PIR goes LOW
          if (debug) { Serial.println("-> idle - no bottom trigger"); }
          noSecondSensor(!startedAtTop, 50, 50, 80);
        }
      }
     break;
    case bottomTriggeredState:            // bottom saw motion first
      if (top) {                          // top see anything yet?
        if (debug) { Serial.println("-> waitingTimeout bottom"); }
        secondSensorTripped(now, 128, 0, 128);
      } else {                            // upper hasn't tripped yet, timeout yet?
        if (((now - onTime)/1000 >= timeBetweenPIRTriggers) /*|| !bottom*/) { // maybe comment out "|| !bottom"
                                                                          // which forces lights to stay on for time
                                                                          // period rather than when the PIR goes LOW
          if (debug) { Serial.println("-> idle - no top trigger"); }
          noSecondSensor(!startedAtTop, 80, 80, 0);
        }
      }
     break;
    case waitingTimeoutState:             // both PIRs sensed motion, wait for both 
                                          // to reset & time to elapse
      if ((!top) && (!bottom) && (((now - onTime)/1000) >= minimumOnTime)) {
        if (debug) { 
          Serial.print("-> waitingTimeout - done; top: "); Serial.print(top);
          Serial.print(" bottom: "); Serial.println(bottom); Serial.println(" "); 
        }
        setIndicator(0, 0, 0);
        currentAnimation->Finish(startedAtTop);
        currentState = idleState;
      }
      break;
    case failedTimeoutState:              // only one sensor saw motion
      if ((!top) && (!bottom)) {          // reset to idle at some point
        currentState = idleState;
      }
      break;
  }
}

/************************************************************************************
 * standard arduino loop() function
 ************************************************************************************/
void loop() {
  unsigned long now = millis();           // we'll eventually need this multiple times
  processLightLevel(now);
  aRandomNumber = random(0, 9876543);     // do this a lot so that numbers end up being more randomish
  bool top = topPIR.read();
  bool bottom = bottomPIR.read();
  if ((top != prevTop) || (bottom != prevBot)) {
    if (debug) { Serial.print("PIR states top: "); Serial.print(top); Serial.print(", bot: "); Serial.println(bottom); }
    prevTop = top;
    prevBot = bottom;
  }

  executeStateMachine(top, bottom, now);  // keep state machine going

  currentAnimation->Continue();           // keep animation going
  indicatorContinue();                    // and any indicator in use
  blinkActive(now);                       // finally, blink red LED to say we're still running
}

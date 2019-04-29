#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>

/************************************************************************************
 * GPIO pin definitions. We're using all of them on a trinket M0
 ************************************************************************************/
#define TopPIRPin 0         // where upper PIR sensor output is found
#define NeoPixelsPin 1      // pin neopixels are attached to
#define modePin 2           // our display mode setting
#define LightLevelPin 4     // for sensing the light level
#define BottomPIRPin 3      // lower PIR

/************************************************************************************
 * lots of LEDs to light stairs.
 * use the first and last of the strip as indicators for the PIRs
 ************************************************************************************/
const uint16_t LEDCount = 111;     // # LEDs we're working with

#define firstLEDToUse 1     // first LED address of those to use for lighting stairs
const uint16_t lastLEDToUse = LEDCount-2;     // last LED address of those to use for lighting stairs
#define bottomIndicatorLED 0  // LED address to use for bottom PIR indicator
const uint16_t topIndicatorLED = LEDCount-1;  // LED address to use for top PIR indicator

#define minimumOnTime 6     // amount of time (seconds) LEDs stay on after 2nd PIR is triggered
#define timeBetweenPIRTriggers 12 // time out period when waiting for 2nd PIR to trip

/************************************************************************************
 * state definitions for our pseudo state machine implementation
 ************************************************************************************/
#define idleState 0         // waiting for a PIR to trigger
#define topTriggered 1      // top one triggered, waiting for bottom
#define bottomTriggered 2   // bottom one triggered, waiting for top
#define waitingTimeout 3    // both triggered, waiting for time out
#define failedTimeout 4     // one PIR tripped, other never did

bool debug = false;         // set to true to get lots of debug printouts
bool debugPattern = false;   // set to true when testing a pattern - ignores startup checks that PIRs are LOW

/************************************************************************************
 * light levels to used to decide which color to light the stairs
 ************************************************************************************/
#define klightLevelBright 875
int lightLevelBright = klightLevelBright;  // above this, brightish white
#define klightLevelMedium 500
int lightLevelMedium = klightLevelMedium;  // below this, a dimmer pale blue
#define klightLevelDim 225
int lightLevelDim = klightLevelDim;     // below this, a dimish red
#define klightLevelTwinkleThreshold 600
int lightLevelTwinkleThreshold = klightLevelTwinkleThreshold;  // separator between twinkle mode & fade mode

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDCount, NeoPixelsPin, NEO_GRB + NEO_KHZ800);
Adafruit_DotStar dot = Adafruit_DotStar(1, INTERNAL_DS_DATA, INTERNAL_DS_CLK, DOTSTAR_BGR);

/************************************************************************************
 * some named colors to use in various places in the code
 ************************************************************************************/
uint32_t onColor = strip.Color(255, 255, 255);      // color to use for ON
uint32_t offColor = strip.Color(0, 0, 0);           // color to use for OFF
uint32_t indicatorColor = strip.Color(10, 10, 10);  // color to use for indicator LEDs
uint32_t dimRed = strip.Color(75, 0, 0);
uint32_t dimPaleBlue = strip.Color(50, 50, 128);

#define colorTableSize 14
/************************************************************************************
 * table of colors to use in chooseColor
 ************************************************************************************/
uint32_t colors[colorTableSize] = {
  strip.Color(255,200,100),       // white-ish
  strip.Color(0x00, 0x80, 0x00),  // pale green
  strip.Color(0xFF, 0x00, 0x52),  // rosy
  strip.Color(0x00, 0x00, 0x80),  // light blue
  strip.Color(255, 255, 0),       // yellow
  strip.Color(255, 0, 255),       // magenta
  strip.Color(0, 255, 255),       // cyan
  strip.Color(128, 128, 255),     // pale blue
  strip.Color(0x9E, 0x1E, 0x00),  // orange
  strip.Color(0x77, 0x00, 0xA9),  // violet
  strip.Color(0x00, 0x85, 0x82),  // pale cyan
  strip.Color(0xFF, 0x44, 0x44),  // pink-ish
  strip.Color(50, 50, 128),       // dimPaleBlue
  strip.Color(0, 0xDD, 0x15)      // mostly green
};

// below are used to attempt to normalize readings from the light sensors
int lastLevel = 0;                // last actual light level reading (use when LEDs on)
int lightLevelMin = 1024;         // lowest light level seend during some period
int lightLevelMax = 0;            // highest light level seen during some period
unsigned long LightLevelSamplingStartTime = 0;
bool LEDsAreOn = false;           // used to avoid taking a light reading while LEDS on

/************************************************************************************
 * gets the current light level; except:
 *   if the LEDs are on, it returns previous reading from when they weren't on
 *   if the level hasn't changed very much, hands back previous reading
 ************************************************************************************/
int getLightLevel() {
  if (LEDsAreOn) {
    if (debug) { Serial.println("LEDsAreOn is set"); }
    return lastLevel;             // they will likely foul the reading, just return last one
  }
  int level = analogRead(LightLevelPin);
  if (level > lightLevelMax) {
    lightLevelMax = level;
  }
  if (level < lightLevelMin) {
    lightLevelMin = level;
  }
  if (abs(level-lastLevel) > 3) {
    if (debug) { Serial.print("light level: "); Serial.println(level); }
    lastLevel = level;
  }
  return level;
}

/************************************************************************************
 * attempt to rescale the light thresholds based on readings seen over some time period
 ************************************************************************************/
void remapLightLevelThresholds() {
    if (debug) { Serial.println("remapLightLevelThresholds"); }
    lightLevelBright = map(klightLevelBright, 0, 1024, 0, lightLevelMax);
    lightLevelMedium = map(klightLevelMedium, 0, 1024, 0, lightLevelMax);
    lightLevelDim = map(klightLevelDim, 0, 1024, 0, lightLevelMax);
    lightLevelTwinkleThreshold = map(klightLevelTwinkleThreshold, 0, 1024, 0, lightLevelMax);
}

/************************************************************************************
 * pick an random color (and check it's not the same as last time)
 ************************************************************************************/
int previousColorIdx = 0;

uint32_t randomColor() {
  int idx = random(0, colorTableSize);
  if (idx == previousColorIdx) {   // same color as last time?
    idx += 1;                   // yes, use next color
    if (idx >= colorTableSize) {
      idx = 0;                  // wrap rather than go off end
    }
  }
  previousColorIdx = idx;       // for next time
  return colors[idx];
}

/************************************************************************************
 * pick an appropriate color based on mode and ambient light conditions
 ************************************************************************************/
uint32_t chooseColor(bool fading=false) {
    uint32_t theColor = randomColor();      // get a random color
  if (fading) {                             // fading, just use that color
    if (debug) { Serial.print("cc: random color: "); Serial.println(theColor); }
    return theColor;
  } else {                                  // otherwise, pick something based on light levels
    int lightLevel = getLightLevel();
//    theColor = onColor;                   // assume we need bright lights   
    if (lightLevel > lightLevelBright) {
      theColor = offColor;                  // it's pretty bright, no need to light it
    }
    if (lightLevel < lightLevelMedium) {    // somewhat dim, use a dimish pale blue
      theColor = dimPaleBlue;
    }
    if (lightLevel < lightLevelDim) {       // quite dim, use a semi-dim red
      theColor = dimRed;
    }
   if (debug) { Serial.print("cc light level: "); Serial.print(lightLevel); Serial.print(" color: "); Serial.println(theColor); }
   return theColor;
  }
}

/************************************************************************************
 * like .fill(), except that it avoids the indicator LEDs
 * caller should manage brightness
 ************************************************************************************/
void setAllPixelsTo(uint32_t aColor) {
  for (uint16_t i=firstLEDToUse; i<=lastLEDToUse; i++) {
    strip.setPixelColor(i, aColor);
  }
  strip.show();
  LEDsAreOn = (aColor != offColor);
}

/************************************************************************************
 * All LEDs fade to a color; call fadeColorContinue from main loop to continue fade
 ************************************************************************************/
float fadeColorTime = 1.3;      // seconds to accomplish the fade
int fadeColorWaitTime = 0;      // millisecs between fade steps
uint32_t fadeColorIn = 0;
uint32_t fadingColor = 0;
unsigned long fadeColorLastTime = 0;
int fadeColorBrightness = 0;
int fadeColorInc = 0;
bool fadeColorRunning = false;

void fadeToColor(uint32_t theColor) {
  LEDsAreOn = true;
  fadeColorRunning = true;
  fadeColorWaitTime = int(round(fadeColorTime/float(255)*1000));
  fadingColor = theColor;
  fadeColorBrightness = 0;
  fadeColorInc = 1;
  if (fadingColor == offColor) {   // fading to black?
    fadeColorBrightness = 255;  // yes, start at full brightness
    fadeColorInc = -1;          // and fade out
    fadingColor = fadeColorIn;  // strip.getPixelColor(firstLEDToUse);
  } else {
    fadeColorIn = fadingColor;  // remember color so we can fade it out
  }
  fadeColorLastTime = 0;
  fadeToColorContinue();
}

bool fadeToColorContinue() {
  if (!fadeColorRunning) {
    return false;
  }
  unsigned long now = millis();
  if (int(now - fadeColorLastTime) < fadeColorWaitTime) {
    return true;
  }
  strip.setBrightness(fadeColorBrightness); // seem to need to set both color and brightness
  setAllPixelsTo(fadingColor); // to get things to work
  fadeColorBrightness += fadeColorInc;  
  if ((fadeColorBrightness > 255) || (fadeColorBrightness < 0)) {
    fadeColorRunning = false;
  }
  if (!fadeColorRunning) {
    setAllPixelsTo(fadingColor);  // finalize the color
    LEDsAreOn = fadingColor != offColor;
    strip.setBrightness(255);   // turn brightness back on so indicators are visible
    strip.show();
  }
  fadeColorLastTime = now;
  return fadeColorRunning;
}

bool fadeToColorActive() {
  return fadeColorRunning;
}

/************************************************************************************
 * Lights the leds one after the other with a color
 * returns immediately; call wipeColorContinue from main loop to keep it going
 ************************************************************************************/
float wipeTime = 1.3;           // time (in seconds) to use to transition all LEDs
int wipeInterval = 0;           // time (millisecs) betwen steps
int wipeLEDIdx = 0;
int wipeInc = 0;
uint32_t wipeColor = 0;
bool wipeRunning = false;
unsigned long wipeLastTime = 0;

void colorWipe(uint32_t theColor, bool reverse) {
  wipeRunning = true;
  float waitFraction = wipeTime/float(LEDCount-3);// -3 b/c two indicators + last loop wait doesn't count
  wipeInterval = int(round(waitFraction*1000));
  if (debug) { Serial.print("colorWipe "); Serial.print(theColor); Serial.print(" "); Serial.print(reverse); 
               Serial.print("  wipe interval: "); Serial.println(wipeInterval); }
  wipeLEDIdx = firstLEDToUse;
  wipeInc = 1;
  if (reverse) {
    wipeLEDIdx = lastLEDToUse;
    wipeInc = -1;
  }
  wipeColor = theColor;
  strip.setBrightness(255);
  wipeLastTime = millis();
  strip.setPixelColor(wipeLEDIdx, wipeColor);
  strip.show();
  LEDsAreOn = true;
  if (debug) { 
    Serial.print("Wipe wait: "); Serial.print(wipeInterval); Serial.print(", Wipe start time: "); Serial.println(millis());
  }
}

bool wipeColorContinue() {
  if (!wipeRunning) {
    return false;
  }
  unsigned long now = millis();
  int elapsed = (now-wipeLastTime);
  if (elapsed < wipeInterval) {
    return true;
  }
  wipeLEDIdx += wipeInc;
  if ((wipeLEDIdx > lastLEDToUse) || (wipeLEDIdx < firstLEDToUse)) {
    wipeRunning = false;
    LEDsAreOn = (wipeColor != offColor);
    return false;
  }
  strip.setPixelColor(wipeLEDIdx, wipeColor);
  strip.show();
  wipeLastTime = now;
  return true;
}

bool wipeColorActive() {
  return wipeRunning;
}

/************************************************************************************
 * "Twinkle" on/off LEDs; chooses a random color & random LEDs & lights it
 * loops this way until all LEDs lit/off, or an amount of time has passed
 * then loops over LEDs determining which haven't been done yet & handles them
 * once everything is ON, then we twinkle some off/on until we're turned off
 * returns immediately; call twinkleContinue from main loop to keep it going
 ************************************************************************************/

bool LEDArray[LEDCount];      // for twinkling state (which LEDs have been twinkled)
float twinkleTime = 1.75;     // do full thing in this many seconds
int twinkleInterval = 0;
unsigned long twinkleLastTime = 0;
bool twinkleRunning = false;
bool twinkleSome = false;       // true if lit up and time to twinkle a few off and on
unsigned long twinkleLongestTimeToWait = 0;
int twinkleChangedCount = 0;
bool twinkleOnState = false;
int lastTwinkleCount = 0;

void twinkle(bool turnOn) {
  strip.setBrightness(255);
  twinkleRunning = true;
  twinkleOnState = turnOn;
  LEDsAreOn = true;
  for (int i=firstLEDToUse; i<=lastLEDToUse; i++) {
    LEDArray[i] = !turnOn;  // initialize to opposite state that we want
  }
  float waitFraction = (twinkleTime/float(LEDCount-3)/1.5);
  twinkleInterval = int(round(waitFraction*1000));                  // time between steps
  twinkleLongestTimeToWait = int(round(twinkleTime*1000))+millis(); // max time for pattern
  twinkleChangedCount = 2;  // account for indicator LEDs
  if (debug) { Serial.print("twinkle: "); Serial.println(turnOn); }
  strip.setBrightness(255);
  twinkleLastTime = 0;
  twinkleSome = false;
  lastTwinkleCount = 0;
  twinkleContinue();       // do one now
}

// sometimes we're finished, but not all LEDs have been transitioned to a color or OFF
// we clean that up here & maybe initialize twinkling
void twinkleFinish() {     // wrap things up (maybe)
  for (int i=firstLEDToUse; i<=lastLEDToUse; i++) {
    if (LEDArray[i] != twinkleOnState) {
      uint32_t aColor = randomColor();
      if (!twinkleOnState) {
        aColor = offColor;
      }
      strip.setPixelColor(i, aColor);
    }
  }
  strip.show();
  twinkleRunning = twinkleOnState; // are we done yet?
  if (twinkleRunning) {
    twinkleInterval = twinkleInterval*2;
    twinkleSomeInit();    // transition to the twinkling some LEDs phase
  }
  LEDsAreOn = twinkleOnState;
}

int MAXTOTWINKLE = LEDCount/10;
int twinkleIndices[LEDCount/10];
int twinkleIdx = 0;
bool twinkleToOn = false;

// setup to twinkle a few of the LEDs
void twinkleSomeInit() {  // setup to twinkle some LEDs
  for (int i=0; i<MAXTOTWINKLE; i++) {
    twinkleIndices[i] = random(firstLEDToUse, lastLEDToUse-1);
  }
  twinkleSome = true;     // set the mode
  twinkleToOn = false;    // start by turning LEDs off
  twinkleIdx = 0;         // reset our poiner
}

// twinkle (turn off then back on) a few LEDs ~10% of strip
void twinkleSomeLEDs() {
  uint32_t aColor = offColor; // assume turn off
  if (twinkleToOn) {      // but we're turning it back on
    aColor = randomColor(); // get a new color
  }
  strip.setPixelColor(twinkleIndices[twinkleIdx], aColor);
  strip.show();
  if (twinkleToOn) {      // if back on, change the pixel we'll do next time
    twinkleIndices[twinkleIdx] = random(firstLEDToUse, lastLEDToUse);
  }
  twinkleIdx += 1;        // walk over our "buffer"
  if (twinkleIdx >= MAXTOTWINKLE) {
    twinkleIdx = 0;       // wrap around
    twinkleToOn = !twinkleToOn; // and change on/off modes
  }
  twinkleLastTime = millis();
}

// do next step in lighting or turning off the LEDs
bool twinkleContinue() {
  if (!twinkleRunning) {  // active?
    return false;
  }
  unsigned long now = millis();
  if (int(now - twinkleLastTime) < twinkleInterval) {
    return true;          // active, but not yet time for next step
  }
  if (twinkleSome) {      // twinkling mode?
    twinkleSomeLEDs();    // yep
    return true;
  }
  if (twinkleChangedCount >= LEDCount) {
    twinkleFinish();
    return twinkleRunning;
  }
  if (now > twinkleLongestTimeToWait) {   // exceeded allotted time to do pattern?
    twinkleFinish();
    return twinkleRunning;
  }
  int tryIdx = random(firstLEDToUse, LEDCount); // try a random LED
  if (LEDArray[tryIdx] == twinkleOnState) {    // already transitioned?
    int origTryIdx = tryIdx;            // then search for next non transitioned LED
    do {
      tryIdx += 1;
      if (tryIdx > lastLEDToUse) {
        tryIdx = firstLEDToUse;
      }
    } while ((LEDArray[tryIdx] == twinkleOnState) && (tryIdx != origTryIdx));
  }
  twinkleChangedCount += 1;
  uint32_t aColor = randomColor();
  if (!twinkleOnState) {
    aColor = offColor;
  }
  LEDArray[tryIdx] = twinkleOnState;
  strip.setPixelColor(tryIdx, aColor);
  strip.show();
  twinkleLastTime = now;
  return true;
}

bool twinkleActive() {
  return twinkleRunning;
}

/************************************************************************************
 * zip a single LED back and forth
 * Returns immediately, call zipLineContinue in main loop to keep it going
************************************************************************************/
int zipLinePixel = firstLEDToUse;
uint32_t zipLineColor = dimPaleBlue;
int zipLineInc = 0;
unsigned long zipLineTime = 0;
bool zipLineRunning = false;
float zipLinePatternTime = 2.0;
int zipLinePatternInterval = 0;

void zipLine(uint32_t colorToUse, bool turnOn, bool fromBottom) {
  float waitFraction = zipLinePatternTime/float(LEDCount-3);// -3 b/c two indicators + last loop wait doesn't count
  zipLinePatternInterval = int(round(waitFraction*1000));
  if (debug) { Serial.print("zipLinePatternInterval "); Serial.println(zipLinePatternInterval); }
  strip.setBrightness(255);
  if (turnOn) {
    zipLineColor = colorToUse;
    zipLineRunning = true;
    zipLinePixel = lastLEDToUse;
    zipLineInc = -1;
    if (fromBottom) {
      zipLinePixel = firstLEDToUse;
      zipLineInc = 1;
    } 
    strip.setPixelColor(zipLinePixel, zipLineColor);
    strip.show();
    zipLineTime = millis();
  } else {
    setAllPixelsTo(offColor);
    zipLineRunning = false;
  }
}

bool zipLineContinue() {
  if (!zipLineRunning) {
    return false;         // we're not doing anything - sleep there
  }
  unsigned long now = millis();
  if (int(now-zipLineTime) < zipLinePatternInterval) {
    return true;          // we're running, no sleep for the wicked
  }
  strip.setPixelColor(zipLinePixel, offColor);
  zipLinePixel += zipLineInc;
  if (zipLinePixel > lastLEDToUse) {
      zipLinePixel = lastLEDToUse-1;  // just turned off last, so move one
      zipLineInc = -zipLineInc;
  } else if (zipLinePixel < firstLEDToUse) {
    zipLinePixel = firstLEDToUse+1;   // just turned off first, so move one
    zipLineInc = -zipLineInc;
  }
  strip.setPixelColor(zipLinePixel, zipLineColor);
  strip.show();
  zipLineTime = now;
  return true;
}

bool zipLineActive() {
  return zipLineRunning;
}

/************************************************************************************
 * zip a single off LED back and forth
 * Returns immediately, call zipLineContinue in main loop to keep it going
************************************************************************************/
bool zipLineInverseRunning = false;

void zipLineInverse(uint32_t colorToUse, bool turnOn, bool fromBottom) {
  float waitFraction = zipLinePatternTime/float(LEDCount-3);// -3 b/c two indicators + last loop wait doesn't count
  zipLinePatternInterval = int(round(waitFraction*1000));
  if (debug) { Serial.print("zipLineInversePatternInterval "); Serial.println(zipLinePatternInterval); }
  if (turnOn) {
    strip.setBrightness(64);
    zipLineColor = colorToUse;
    zipLineInverseRunning = true;
    zipLinePixel = lastLEDToUse;
    zipLineInc = -1;
    if (fromBottom) {
      zipLinePixel = firstLEDToUse;
      zipLineInc = 1;
    }
    for (int i=firstLEDToUse; i<=lastLEDToUse; i++) {
      strip.setPixelColor(i, zipLineColor);
    }
    strip.show();
    zipLineTime = millis();
  } else {
    strip.setBrightness(255);
    setAllPixelsTo(offColor);
    zipLineInverseRunning = false;
  }
}

bool zipLineInverseContinue() {
  if (!zipLineInverseRunning) {
    return false;         // we're not doing anything - sleep there
  }
  unsigned long now = millis();
  if (int(now-zipLineTime) < zipLinePatternInterval) {
    return true;          // we're running, no sleep for the wicked
  }
  strip.setPixelColor(zipLinePixel, zipLineColor);
  zipLinePixel += zipLineInc;
  if (zipLinePixel > lastLEDToUse) {
      zipLinePixel = lastLEDToUse;  // just turned off last, so move one
      zipLineInc = -zipLineInc;
  } else if (zipLinePixel < firstLEDToUse) {
    zipLinePixel = firstLEDToUse;   // just turned off first, so move one
    zipLineInc = -zipLineInc;
  }
  strip.setPixelColor(zipLinePixel, offColor);
  strip.show();
  zipLineTime = now;
  return true;
}

bool zipLineInverseActive() {
  return zipLineInverseRunning;
}

/************************************************************************************
 * color swirl - rainbow colors that swirl over time
 * Returns immediately, call colorSwirlContinue from main loop to keep it going
 * NB: this code is basically transcribed from an Adafruit example in circuitpython
 *     with some minor refactoring to allow other processing while it's happening
 ************************************************************************************/
uint32_t wheel(int pos) {
  if ((pos < 0) || (pos > 255)) {
    return offColor;
  }
  if (pos < 85) {
    return strip.Color(255-pos*3, pos*3, 0);
  }
  if (pos < 170) {
    pos -= 85;
    return strip.Color(0, 255-pos*3, pos*3);
  }
  pos -= 170;
  return strip.Color(pos*3, 0, 255-pos*3);
}

int colorSwirlIndex = 0;
int colorSwirlInc = 0;
unsigned long colorSwirlTime = 0;
bool colorSwirlRunning = false;

void colorSwirl(bool turnOn) {
  strip.setBrightness(255);
  if (turnOn) {
    colorSwirlRunning = true;
    colorSwirlIndex = 0;
    colorSwirlInc = 1;
    for (int i=firstLEDToUse; i<=lastLEDToUse; i++) {
      int rc_index = (i * 256 / LEDCount-2) + colorSwirlIndex;
      strip.setPixelColor(i, wheel(rc_index & 255));
    }
    strip.show();
    colorSwirlTime = millis();
  } else {
    setAllPixelsTo(offColor);
    colorSwirlRunning = false;
  }
}

bool colorSwirlContinue() {
  if (!colorSwirlRunning) {
    return false;
  }
  if (millis()-colorSwirlTime < 10) {
    return true;
  }
  colorSwirlIndex += colorSwirlInc;
  if (colorSwirlIndex > 255) {
    colorSwirlIndex = 254;
    colorSwirlInc = -colorSwirlInc;
  }
  if (colorSwirlIndex < 0) {
    colorSwirlIndex = 1;
    colorSwirlInc = -colorSwirlInc;
  }
  for (int i=firstLEDToUse; i<lastLEDToUse; i++) {
    int rc_index = (i * 256 / LEDCount-2) + colorSwirlIndex;
    strip.setPixelColor(i, wheel(rc_index & 255));
  }
  strip.show();
  colorSwirlTime = millis();
  return true;
}

bool colorSwirlActive() {
  return colorSwirlRunning;
}

/************************************************************************************
 * marquee lights
 * light every nth light and then prescess
 ************************************************************************************/
bool marqueeRunning = false;
uint32_t marqueeColor = 0;
int marqueeQuanta = 5;
int marqueeOffset = 0;
int marqueeInc = 1;
int marqueeInterval = 150;
unsigned long marqueeLastUpdate = 0;

bool marquee(uint32_t colorToUse, bool turnOn, bool fromBottom) {
  if (!turnOn) {
    strip.setBrightness(255);
    setAllPixelsTo(offColor);
    marqueeRunning = false;
    return false;
  }
  marqueeInc = -1;
  if (fromBottom) {
    marqueeInc = 1;
  }
  marqueeQuanta = 4;
  marqueeOffset = 0;
  marqueeLastUpdate = 0;
  marqueeRunning = true;
  marqueeColor = colorToUse;
  strip.setBrightness(64);
  marqueeContinue();
  return true;
}

bool marqueeContinue() {
  if (!marqueeRunning) {
    return false;
  }
  unsigned long now = millis();
  if (int(now - marqueeLastUpdate) < marqueeInterval) {
    return true;
  }
  LEDsAreOn = true;
  for (int i=firstLEDToUse; i<=lastLEDToUse; i++) {
    strip.setPixelColor(i, marqueeColor);
  }
  for (int i=firstLEDToUse+marqueeOffset; i<=lastLEDToUse; i = i + marqueeQuanta) {
    strip.setPixelColor(i, offColor);
  }
  strip.show();
  marqueeOffset += marqueeInc;
  if (marqueeOffset < 0) {
    marqueeOffset = marqueeQuanta-1;
  }
  if (marqueeOffset >= marqueeQuanta) {
    marqueeOffset = 0;
  }
  marqueeLastUpdate = now;
  return true;
}

bool marqueeActive() {
  return marqueeRunning;
}

/************************************************************************************
 * decide what lighting scheme to use
 * originally based on light level, now random
 ************************************************************************************/
static int kMaxFadeCount = 5;
int lastFadeIndex = 0;

void doFadeOr(bool turnOn, uint32_t colorToUse, bool topToBottom) {
  int fadeToUse = lastFadeIndex;
  if (turnOn) {
    fadeToUse = random(0, kMaxFadeCount);
    if (fadeToUse == lastFadeIndex) {
      fadeToUse += 1;
      if (fadeToUse >= kMaxFadeCount) {
        fadeToUse = 0;
      }
    }
  }
  lastFadeIndex = fadeToUse;
  switch (fadeToUse) {
    case 0:
      twinkle(turnOn);
      break;
    case 1:
      colorSwirl(turnOn);
      break;
    case 2:
      marquee(colorToUse, turnOn, !topToBottom);
      break;
    case 3:
      zipLineInverse(colorToUse, turnOn, !topToBottom);
      break;
    default:
      if (turnOn) {
        fadeToColor(colorToUse);
      } else {
        fadeToColor(offColor);
      }
      break;
  }
}

/********************************************************************* 
 * wrapper function to turn on/off the leds 
 * chooses a color, then calls either colorWipe or fadeToColor based
 * on mode line
*********************************************************************/
bool lastFadeMode = false;

void displayLights(bool turnOn, bool topToBottom=false) {
  if (displayRunning() && turnOn) {
    return;
  }
  bool fadeMode = (digitalRead(modePin) == LOW);
  if (!turnOn) {              // use fade mode that was active at turnOn=true time
    fadeMode = lastFadeMode;
  } else {
    lastFadeMode = fadeMode;
  }
  if (debug) { Serial.print("dl: "); Serial.print(turnOn); Serial.print(" fm: "); Serial.print(fadeMode);
  Serial.print(" ttb: "); Serial.println(topToBottom); }
  uint32_t colorToUse = chooseColor(fadeMode);
  if (fadeMode) {
    doFadeOr(turnOn, colorToUse, topToBottom);
  } else {
    if (turnOn) {
      if (colorToUse == offColor) {           // going to show it as OFF?
        uint32_t nextColor = randomColor();   // yes, do a zipLine to show we're operating
        zipLine(nextColor, true, !topToBottom);
      } else {
        colorWipe(colorToUse, topToBottom);   // have a color, wipe it
      }
    } else {
      if (zipLineRunning) {
        zipLine(offColor, false, !topToBottom); 
      } else {
        colorWipe(offColor, topToBottom);
      }
    }
  }
}

/************************************************************************************
 * pair of functions to read the PIR status
 * done this way to also show an indicator of when the PIR is tripped
 * PLUS, force an OFF return if the PIR seems stuck ON for too long
 * 
 * we only update the LED indicators on state change; otherwise the
 * show() required to keep it up to date slows things down too much
 * 
 * should have a structure and single routine to do this, BUT
 * on the M0 trinket, we're currently using < 10% of space, so...
 ************************************************************************************/
unsigned long topWentHighAt = 0;
int lastTopPIRState = LOW;

int readTopPIR(bool rawRead=false) {
  int state = digitalRead(TopPIRPin);
  if (state != lastTopPIRState) {               // did the state change?
    uint32_t clr = (state == HIGH) ? indicatorColor : offColor;
    strip.setPixelColor(topIndicatorLED, clr);  // yes, show that
    strip.show();   // should show, but slows things down, let other shows accomplish this  
  }
  if (state == HIGH) {
    if (topWentHighAt == 0) {
      topWentHighAt = millis();
    } else if (((millis() - topWentHighAt) > 15000) && (!rawRead)) {      // is PIR stuck ON?
      return LOW;                                         // fake that it went OFF
    }
  } else if (topWentHighAt != 0) {
    if (debug) { Serial.print("   Top PIR HIGH for "); Serial.println(millis()-topWentHighAt); }
    topWentHighAt = 0;
  }
  lastTopPIRState = state;
  return state;
}

/************************************************************************************
 * read the lower PIR state; comments as above
 ************************************************************************************/
unsigned long bottomWentHighAt = 0;
int lastBottomPIRState = LOW;

int readBottomPIR(bool rawRead=false) {
  int state = digitalRead(BottomPIRPin);
  if (state != lastBottomPIRState) {
    uint32_t clr = (state == HIGH) ? indicatorColor : offColor;
    strip.setPixelColor(bottomIndicatorLED, clr);
    strip.show();   // only set color & show if state changed, slows things down otherwise
  }
  if (state == HIGH) {
    if (bottomWentHighAt == 0) {
      bottomWentHighAt = millis();
    } else if (((millis() - topWentHighAt) > 15000) && (!rawRead)) {      // is PIR stuck ON?
      return LOW;                                           // fake that it went OFF
    }
  } else if (bottomWentHighAt != 0) {
    if (debug) { Serial.print("Bottom PIR HIGH for "); Serial.println(millis()-bottomWentHighAt); }
    bottomWentHighAt = 0;
  }
  lastBottomPIRState = state;
  return state;
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
  if ((now - lastTime) >= 500) {        // blink RED LED to show we're active
    if (useDotStar) {                   // used occasionally (like at start up)
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

bool displayRunning() {
  return zipLineActive() || colorSwirlActive() || wipeColorActive() || twinkleActive() || 
          fadeToColorActive() || marqueeActive() || zipLineInverseActive();
}

// call all the pattern continuation functions
void continueLighting() {
    zipLineContinue();
    colorSwirlContinue();
    wipeColorContinue();
    twinkleContinue();
    fadeToColorContinue();
    indicatorContinue();
    marqueeContinue();
    zipLineInverseContinue();
}

/************************************************************************************
 * standard arduino setup function
 *  we enable various pins we use for input/output
 *  including the one that controls the neopixel strip
 ************************************************************************************/
unsigned long debugPatternTime = 0;
bool debugPatternOn = false;

void setup() {
  LightLevelSamplingStartTime = millis();
  Serial.begin(9600);
  for (int i=0; i<10; i++) {    // stall for a bit to get serial working
    Serial.print(".");
    delay(250);
  }
  randomSeed(getLightLevel());
  strip.begin();                // setup the LEDs to light the stairs
  strip.show();                 // Initialize all pixels to 'off'
  dot.begin();                  // set up indicator dotstar
  dot.show();
  pinMode(TopPIRPin, INPUT);    // input for PIR at top of the stairs
  pinMode(BottomPIRPin, INPUT); // PIR at bottom of stairs
  pinMode(modePin, INPUT_PULLUP); // mode switch (fade vs running lights)
  Serial.print("\nWaiting for PIR's to stabilize");
  setIndicator(128, 128, 0);
  int top, bottom;
  bool state = false;
  if (!debugPattern) {
    do {                          // check the PIR lines until they are both LOW (stabilized)
      delay(250);
      if (state) {
        dot.setPixelColor(0, 0, 0, 0);
      } else {
        dot.setPixelColor(0, 128, 96, 0);
      }
      dot.show();
      state = !state;
      top = readTopPIR(true);
      bottom = readBottomPIR(true);
      strip.show();
    } while ((top == HIGH) || (bottom == HIGH));
    Serial.println("  Started");
    setIndicator(0, 0, 0);
  } else {
    Serial.println("  Started in debugPattern mode");
    setIndicator(64, 92, 64);
    //marquee(randomColor(), true, true);
    zipLineInverse(randomColor(), true, true);
    debugPatternTime = millis();
    debugPatternOn = true;
  }
/*
  Serial.print("Bottom: "); Serial.print(bottomIndicatorLED); Serial.print(", top: "); Serial.println(topIndicatorLED);
  Serial.print("First: "); Serial.print(firstLEDToUse); Serial.print(", last: "); Serial.println(lastLEDToUse);
*/
}

/************************************************************************************
 * standard arduino loop function
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
unsigned long onTime = 0;     // remember time we turned leds on
unsigned long lastLightReadTime = 0;
bool firstDay = true;

bool startedAtTop = false;    // remember if the PIR at the top tripped first (used rather than expanding the state machine)
int currentState = idleState; // current state in pseudo-state machine

void loop() {
  int TopPIRState = 0;
  int BottomPIRState = 0;

  unsigned long now = millis();
  if (debugPattern) {
    if ((now - debugPatternTime) > 6000) {
      if (debugPatternOn) {
        //marquee(offColor, false, false);
        zipLineInverse(offColor, false, false);
        debugPatternOn = false;
      } else {
        //marquee(randomColor(), true, false);
        zipLineInverse(randomColor(), true, false);
        debugPatternOn = true;
      }
      debugPatternTime = now;
    }
    goto debugPatternLabel;
  }
 // this first bit is an attempt to normalize readings from the photoresistor
 
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
 
 // now start dealing with the PIR sensors and our pseudo state machine
 
  TopPIRState = readTopPIR(); // digitalRead(TopPIRPin);
  BottomPIRState = readBottomPIR();
  switch (currentState) {
    case idleState:               // waiting for something to happen
      if (TopPIRState == HIGH) {  // upper PIR tripped?
        if (debug) { Serial.println("-> topTriggered"); }
        setIndicator(128, 0, 0);  // RED
        startedAtTop = true;
        onTime = now;
        displayLights(true, startedAtTop);
        currentState = topTriggered;
      } else if (BottomPIRState == HIGH) {  // lower PIR tripped?
        if (debug) { Serial.println("-> bottomTriggered"); }
        setIndicator(0, 128, 0);  // GREEN
        startedAtTop = false;
        onTime = now;
        displayLights(true, startedAtTop);
        currentState = bottomTriggered;
      }
      break;
    case topTriggered:          // upper PIR went HIGH, waiting for lower PIR to trip
      if (BottomPIRState == HIGH) {
        if (debug) { Serial.println("-> waitingTimeout top"); }
        setIndicator(0, 0, 128);  // BLUE
        onTime = now;
        currentState = waitingTimeout;
        break;
      } else {                // lower PIR not triggered yet; timeout yet?
        if ((now - onTime)/1000 >= timeBetweenPIRTriggers) {
          if (debug) { Serial.println("-> idle - no bottom trigger"); }
          setIndicator(50, 50, 80); // PALE BLUE
          displayLights(false, !startedAtTop);
          currentState = failedTimeout; //waitingTimeout;
        }
      }
      break;
    case bottomTriggered:   // lower PIR triggered, waiting for upper PIR to trip
      if (TopPIRState == HIGH) {
        if (debug) { Serial.println("-> waitingTimeout bottom"); }
        setIndicator(128, 0, 128);  // MAGENTA
        onTime = now;
        currentState = waitingTimeout;
      } else {            // upper hasn't tripped yet, timeout yet?
        if ((now - onTime)/1000 >= timeBetweenPIRTriggers) {
          if (debug) { Serial.println("-> idle - no top trigger"); }
          setIndicator(80, 80, 0);  // YELLOW
          displayLights(false, !startedAtTop);
          currentState = failedTimeout; //waitingTimeout;
        }
      }
      break;
    case waitingTimeout:  // both PIRs have triggered, now waiting to turn off the LEDs
      if ((TopPIRState == LOW) && (BottomPIRState == LOW) && (((now - onTime)/1000) >= minimumOnTime)) {
        if (debug) { Serial.print("-> waitingTimeout - done; top: "); Serial.print(TopPIRState == HIGH);
        Serial.print(" bottom: "); Serial.println(BottomPIRState == HIGH); Serial.println(" "); }
        setIndicator(0, 0, 0);  // BLACK
        displayLights(false, startedAtTop);
        currentState = idleState;
      }
      break;
    case failedTimeout:   // one PIR had triggered, but the other never did and we timed out
      if ((TopPIRState == LOW) && (BottomPIRState == LOW)) {
        currentState = idleState;
      }
    break;
  }
debugPatternLabel:
  blinkActive(now);       // indicate that we're cycling
  continueLighting();
}

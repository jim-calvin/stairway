# CircuitPython version of the Stairway lighting project
# this code is intended to be (roughly) the same as the
# Arduino version.

# one issue, it requires M4 express based board to load
# and execute it.
# The full CircuitPython version will not run on the
# original projects Trinket M0; although a cut down
# version of the CircuitPython code exists that will
# run on the Trinket M0

import time
import board
import neopixel
import digitalio
from Animation import *
import random
import PIR
from indicator import *
import lightLevel

DEBUG = False

# definition for CircuitPlayground express
pixel_pin = board.A0
topPIR_pin = board.A1
bottomPIR_pin = board.A2
modeSwitch_pin = board.A4
photoresistor_pin = board.A3

# for Feather M4 express
pixel_pin = board.D9
topPIR_pin = board.D6
bottomPIR_pin = board.D5
modeSwitch_pin = board.D10
photoresistor_pin = board.A0

num_pixels = 30

# named colors
INDICATOR = (10, 10, 10)
YELLOW = (255, 150, 0)
OFFCOLOR = (0, 0, 0)
WHITE = (200, 200, 200)
DIMRED = (75, 0, 0)
DIMPALEBLUE = (50, 50, 128)

# setup the neopixel strip for use
pixels = neopixel.NeoPixel(pixel_pin, num_pixels, brightness=0.5,
                           auto_write=False)
# on circuitplayground express, use on board neopixels as indicators
dot = neopixel.NeoPixel(board.NEOPIXEL, 1, brightness=0.1)

# on circuitplayground express, setup pins for PIR inputs
topPIR = PIR.PIR(topPIR_pin, pixels, num_pixels-1, INDICATOR, "top")
topPIR.FAKEPIR = True
topPIR.DEBUG = DEBUG
bottomPIR = PIR.PIR(bottomPIR_pin, pixels, 0, INDICATOR, "bottom")
bottomPIR.FAKEPIR = True
bottomPIR.DEBUG = DEBUG

# on circuitplayground express, setup pin for mode switch
modeSwitch = digitalio.DigitalInOut(modeSwitch_pin)
modeSwitch.direction = digitalio.Direction.INPUT
modeSwitch.pull = digitalio.Pull.UP

# on circuitplayground express, setup pin for photoresistor input
lightLevel = lightLevel.lightLevel(photoresistor_pin)
lightLevel.DEBUG = DEBUG

# setup PIR objects
busyIndicator = blinker(dot, 0, 'LED')
busyIndicator.setIndicator(DIMRED)
busyIndicator.DEBUG = DEBUG
stateIndicator = indicator(dot, 0, False)
stateIndicator.setIndicator(YELLOW)
stateIndicator.DEBUG = DEBUG

# setup animation objects
aZipper = ZipLine(pixels, 1.3, 1, num_pixels-2)
aZipper.DEBUG = DEBUG
aColorWipe = ColorWipe(pixels, 1.3, 1, num_pixels-2)
aColorWipe.DEBUG = DEBUG
#aColorWipe = 0
aSwirl = ColorSwirl(pixels, -0.00, 1, num_pixels-2)
aSwirl.DEBUG = DEBUG
aTwinkle = Twinkle(pixels, 1.5, 1, num_pixels-2)
aTwinkle.DEBUG = DEBUG
aZipInverse = ZipLineInverse(pixels, 1.3, 1, num_pixels-2)

# lists of animations to use based on mode switch setting
standardAnimations = [aColorWipe, aZipper]
fadeAnimations = [aSwirl, aColorWipe, aZipper, aTwinkle, aZipInverse]

systemState = {'currentState': 'idleState', 'currentAnimation': aSwirl}

def colorBasedOnConditions():
    aColor = Animation.randomColor()
    if not modeSwitch.value:
        if systemState['currentAnimation'] == aColorWipe:
            lvl = lightLevel.value
            if lvl > lightLevel.lightLevelBright:
                aColor = OFFCOLOR
            if lvl < lightLevel.lightLevelMedium:
                aColor = DIMPALEBLUE
            if lvl < lightLevel.lightLevelDim:
                aColor = DIMRED
    return aColor

def chooseAnimation():
    newAnimation = systemState['currentAnimation']
    while (newAnimation == systemState['currentAnimation']):
        if modeSwitch.value:
            newAnimation = random.choice(fadeAnimations)
        else:
            newAnimation = random.choice(standardAnimations)
    return newAnimation

TIMEBETWEENTRIGGERS = 12
MINIMUMONTIME = 10

def firstSensorTripped(nextState, fromTop, now, indColor):
    ca = chooseAnimation()
    systemState['currentAnimation'] = ca
    if DEBUG:
        ca.printSelf()
    stateIndicator.setIndicator(indColor)
    systemState['fromTop'] = fromTop
    systemState['onTime'] = now
    ca.Start(fromTop, colorBasedOnConditions())
    systemState['currentState'] = nextState

def secondSensorTripped(now, indColor):
    stateIndicator.setIndicator(indColor)
    systemState['onTime'] = now
    systemState['currentState'] = 'waitingTimeoutState'

def noSecondSensor(fromTop, indColor):
    stateIndicator.setIndicator(indColor)
    systemState['currentAnimation'].Finish(fromTop)
    systemState['currentState'] = 'failedTimeoutState'

def executeStateMachine(top, bottom, now):
    state = systemState['currentState']
    if state == 'idleState':
        if top:
            if DEBUG:
                print(now, "topPIR tripped -> topTriggeredState")
            firstSensorTripped('topTriggeredState', True, now, (128, 0, 0))
        if bottom:
            if DEBUG:
                print(now, "bottomPIR tripped -> bottomTriggeredState")
            firstSensorTripped('bottomTriggeredState', False, now, (0, 128, 0))
    elif state == 'topTriggeredState':
        if bottom:
            if DEBUG:
                print(now, "topTriggered + bottom -> waitingTimeoutState")
            secondSensorTripped(now, (0, 0, 128))
        else:
            if ((now - systemState['onTime']) >= TIMEBETWEENTRIGGERS or not top):
                if DEBUG:
                    print(now, "timeout, no bottom detected -> failedTimeoutState")
                noSecondSensor(not systemState['fromTop'], (50, 50, 80))
    elif state == 'bottomTriggeredState':
        if top:
            if DEBUG:
                print(now, "bottomTriggered + top -> waitingTimeoutState")
            secondSensorTripped(now, (128, 0, 128))
        else:
            if ((now-systemState['onTime']) >= TIMEBETWEENTRIGGERS or not bottom):
                if DEBUG:
                    print(now, "timeout, no top detected -> failedTimeoutState")
                noSecondSensor(not systemState['fromTop'], (80, 80, 0))
    elif state == 'waitingTimeoutState':
        if not top and not bottom and (now-systemState['onTime']) >= MINIMUMONTIME:
            if DEBUG:
                print(now, "timeoutComplete -> idleState")
            stateIndicator.setIndicator((0, 0, 0))
            systemState['currentAnimation'].Finish(systemState['fromTop'])
            systemState['currentState'] = 'idleState'
    elif state == 'failedTimeoutState':
        if not top and not bottom:
            if DEBUG:
                print(now, "failedTimeoutState -> idleState")
            systemState['currentState'] = 'idleState'
    else:
        print('unknown state encountered: ', state)

print("Waiting for PIRs to stabilize")
stateIndicator.blinker = True           # temporarily make it a blinker
while topPIR.read() or bottomPIR.read():
    stateIndicator.Continue()

stateIndicator.blinker = False          # return to not being a blinker

print("Ready")

while True:
    now = time.monotonic()
    top = topPIR.read()
    bottom = bottomPIR.read()
    executeStateMachine(top, bottom, now)
    systemState['currentAnimation'].Continue()
    stateIndicator.Continue()
    busyIndicator.Continue()
    lightLevel.processLightLevel(now, systemState['currentAnimation'].Active())
# cut down version of the CircuitPython code to run the Stairway project
# an M0 has insufficient storage and RAM to handle the full code base
# a Feather M4 Express version exists that includes all of the code

import time
import board
import neopixel
import adafruit_dotstar
import digitalio
from Animation import *
import random
import PIR
import lightLevel

DEBUG = False
DEBUG = False

# for Trinket M0
topPIR_pin = board.D0
pixel_pin = board.D1
modeSwitch_pin = board.D2
bottomPIR_pin = board.D4
photoresistor_pin = board.A3
LED_pin = board.D13

num_pixels = 30

# named colors
INDICATOR = (10, 10, 10)
YELLOW = (255, 150, 0)
OFFCOLOR = (0, 0, 0)
DIMRED = (75, 0, 0)
DIMPALEBLUE = (50, 50, 128)

# setup the neopixel strip for use
pixels = neopixel.NeoPixel(pixel_pin, num_pixels, brightness=0.5,
                           auto_write=False)
stateInd = adafruit_dotstar.DotStar(board.APA102_SCK, board.APA102_MOSI, 1)
stateInd.brightness = 0.5
stateInd[0] = YELLOW

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

busy = digitalio.DigitalInOut(LED_pin)
busy.direction = digitalio.Direction.OUTPUT
busy.value = True
busyTimer = time.monotonic()

# setup animation objects
aColorWipe = ColorWipe(pixels, 1.3, 1, num_pixels-2)
aColorWipe.DEBUG = DEBUG

# lists of animations to use based on mode switch setting
# standardAnimations = [aColorWipe]
# fadeAnimations = [aColorWipe]

systemState = {'currentState': 'idleState', 'currentAnimation': aColorWipe}

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
	return aColorWipe;

TIMEBETWEENTRIGGERS = 12
MINIMUMONTIME = 10

def firstSensorTripped(nextState, fromTop, now, indColor):
    ca = chooseAnimation()
    systemState['currentAnimation'] = ca
    if DEBUG:
        ca.printSelf()
    stateInd[0] = indColor
    systemState['fromTop'] = fromTop
    systemState['onTime'] = now
    ca.Start(fromTop, colorBasedOnConditions())
    systemState['currentState'] = nextState

def secondSensorTripped(now, indColor):
    stateInd[0] = indColor
    systemState['onTime'] = now
    systemState['currentState'] = 'waitingTimeoutState'

def noSecondSensor(fromTop, indColor):
    stateInd[0] = indColor
    systemState['currentAnimation'].Finish(fromTop)
    systemState['currentState'] = 'failedTimeoutState'

def executeStateMachine(top, bottom, now):
    state = systemState['currentState']
    if state == 'idleState':
        if systemState['currentAnimation'].Active():
            return      # under unlikely circumstances, we can get a PIR
                        # going HIGH while an animation is finishing
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
            stateInd[0] = (0, 0, 0)
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
while topPIR.read() or bottomPIR.read():
    time.sleep(0.25)
    if stateInd[0] == YELLOW:
        stateInd[0] = OFFCOLOR
    else:
        stateInd[0] = YELLOW

print("Ready")

while True:
    now = time.monotonic()
    top = topPIR.read()
    bottom = bottomPIR.read()
    executeStateMachine(top, bottom, now)
    systemState['currentAnimation'].Continue()
    if now-busyTimer >= 0.25:
        busy.value = not busy.value
        busyTimer = now
    lightLevel.processLightLevel(now, systemState['currentAnimation'].Active())
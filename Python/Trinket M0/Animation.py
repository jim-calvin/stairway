# cut down version of this file to work on a Trinket M0
# see the Feather M4 express version for full set of
# animations

import time
import random

BLACK = (0, 0, 0)

 # table of colors we use
COLORS = [(255, 200, 100), (0x00, 0x80, 0x00), (0xFF, 0x00, 0x52),
          (0x00, 0x00, 0x80), (255, 255, 0), (255, 0, 255),
          (0, 255, 255), (128, 128, 255), (0x9E, 0x1E, 0x00),
          (0x77, 0x00, 0xA9), (0x00, 0x85, 0x82), (0xFF, 0x44, 0x44),
          (50, 50, 128), (0, 0xDD, 0x15), (120, 0, 0)]

class Animation(object):

    DEBUG = False

    @staticmethod
    def randomColor():
        return random.choice(COLORS)

    def __init__(self, pixels, animationTime, firstLED, lastLED):
        self.pixels = pixels
        self.firstLED = firstLED
        self.lastLED = lastLED
        self.colorToUse = BLACK
        self.topToBottom = True
        self.lastUpdateTime = 0
        self.animationTime = animationTime
        if animationTime > 0:
            self.animationStepIncrement = self.animationTime/(self.lastLED-self.firstLED)
        else:
            self.animationStepIncrement = -animationTime
        self.active = False

    def printSelf(self):
        print("base class")

    def Start(self, topToBottom, colorToUse):
        self.runTopToBottom = topToBottom
        self.colorToUse = colorToUse

    def Continue(self):
        pass

    def Active(self):
        return self.active

    def Finish(self, topToBottom):
        pass

    def setAllPixelsTo(self, color):
        for i in range(self.firstLED, self.lastLED+1):
            self.pixels[i] = color

class ColorWipe(Animation):

    def __init__(self, pixels, animationTime, firstLED, lastLED):
        Animation.__init__(self, pixels, animationTime, firstLED, lastLED)
        self.index = 0
        self.inc = 0

    def printSelf(self):
        print("ColorWipe ", self.animationStepIncrement)

    def Start(self, topToBottom, withColor):
        self.active = True
        self.inc = 1
        self.index = self.firstLED
        self.colorToUse = withColor
        self.topToBottom = topToBottom
        if topToBottom:
            self.index = self.lastLED
            self.inc = -1

    def Continue(self):
        if not self.active:
            return
        now = time.monotonic()
        if now-self.lastUpdateTime < self.animationStepIncrement:
            return
        self.pixels[self.index] = self.colorToUse
        self.pixels.show()
        self.index += self.inc
        self.lastUpdateTime = now
        if (self.index > self.lastLED) or (self.index < self.firstLED):
            self.active = False

    def Finish(self, topToBottom):
        self.active = True
        self.colorToUse = (0, 0, 0)
        self.inc = 1
        self.index = self.firstLED
        self.topToBottom = topToBottom
        if topToBottom:
            self.index = self.lastLED
            self.inc = -1

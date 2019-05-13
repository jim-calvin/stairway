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

class ZipLine(Animation):

    def __init__(self, pixels, animationTime, firstLED, lastLED):
        Animation.__init__(self, pixels, animationTime, firstLED, lastLED)
        self.zipIndex = 0
        self.zipInc = 0

    def printSelf(self):
        print("ZipLine ", self.animationStepIncrement)

    def Start(self, topToBottom, colorToUse):
        self.topToBottom = topToBottom
        self.colorToUse = colorToUse
        self.zipIndex = self.firstLED
        self.zipInc = 1
        if (self.topToBottom):
            self.zipIndex = self.lastLED
            self.zipInc = -1
        self.setAllPixelsTo(BLACK)
        self.pixels[self.zipIndex] = self.colorToUse
        self.lastUpdateTime = time.monotonic()
        self.active = True

    def Continue(self):
        if (not self.active):
            return
        now = time.monotonic()
        if (now - self.lastUpdateTime) < self.animationStepIncrement:
            return
        self.pixels[self.zipIndex] = BLACK
        self.zipIndex += self.zipInc
        if (self.zipIndex >= self.lastLED):
            self.zipIndex = self.lastLED
            self.zipInc = -self.zipInc
        if (self.zipIndex < self.firstLED):
            self.zipIndex = self.firstLED+1
            self.zipInc = -self.zipInc
        self.pixels[self.zipIndex] = self.colorToUse
        self.pixels.show()
        self.lastUpdateTime = now

    def Finish(self, topToBottom):
        self.setAllPixelsTo(BLACK)
        self.pixels.show()
        self.active = False

class ZipLineInverse(Animation):

    def __init__(self, pixels, animationTime, firstLED, lastLED):
        Animation.__init__(self, pixels, animationTime, firstLED, lastLED)
        self.zipIndex = 0
        self.zipInc = 0

    def printSelf(self):
        print("ZipLineInverse ", self.animationStepIncrement)

    def Start(self, topToBottom, colorToUse):
        self.topToBottom = topToBottom
        self.colorToUse = colorToUse
        self.zipIndex = self.firstLED
        self.zipInc = 1
        if (self.topToBottom):
            self.zipIndex = self.lastLED
            self.zipInc = -1
        self.setAllPixelsTo(colorToUse)
        self.pixels[self.zipIndex] = BLACK
        self.lastUpdateTime = time.monotonic()
        self.active = True

    def Continue(self):
        if (not self.active):
            return
        now = time.monotonic()
        if (now - self.lastUpdateTime) < self.animationStepIncrement:
            return
        self.pixels[self.zipIndex] = self.colorToUse
        self.zipIndex += self.zipInc
        if (self.zipIndex >= self.lastLED):
            self.zipIndex = self.lastLED
            self.zipInc = -self.zipInc
        if (self.zipIndex < self.firstLED):
            self.zipIndex = self.firstLED+1
            self.zipInc = -self.zipInc
        self.pixels[self.zipIndex] = BLACK
        self.pixels.show()
        self.lastUpdateTime = now

    def Finish(self, topToBottom):
        self.setAllPixelsTo(BLACK)
        self.pixels.show()
        self.active = False

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

class ColorSwirl(Animation):

    def __init__(self, pixels, animationTime, firstLED, lastLED):
        Animation.__init__(self, pixels, animationTime, firstLED, lastLED)
        self.index = 0
        self.inc = 0

    def printSelf(self):
        print("ColorSwirl: ", self.animationStepIncrement)

    def wheel(self, pos):
        if pos < 0 or pos > 255:
            return (0, 0, 0)
        if pos < 85:
            return (pos*3, 255-pos*3, 0)
        if pos < 170:
            pos -= 85
            return (255-pos*3, 0, pos*3)
        pos -= 170
        return (0, pos*3, 255-pos*3)

    def Start(self, topToBottom, colorToUse):
        self.topToBottom = topToBottom
        self.colorToUse     # not used
        self.active = True
        self.index = 254
        self.inc = -1
        if not topToBottom:
            self.index = 0
            self.inc = 1
        self.lastUpdateTime = 0

    def Continue(self):
        if not self.active:
            return
        now = time.monotonic()
        if (now-self.lastUpdateTime) < self.animationStepIncrement:
            return
        self.index += self.inc
        if (self.index > 255):
            self.index = 254
            self.inc = -self.inc
        if (self.index < 0):
            self.index = 1
            self.inc = -self.inc
        for i in range(self.firstLED, self.lastLED+1):
            px_index = (i*256 // (self.lastLED-self.firstLED)) + self.index
            self.pixels[i] = self.wheel(px_index & 255)
        self.pixels.show()
        self.lastUpdateTime = time.monotonic()

    def Finish(self, topToBottom):
        self.topToBottom = topToBottom
        self.setAllPixelsTo(BLACK)
        self.pixels.show()
        self.active = False

class Twinkle(Animation):

    def doAllocate(self, size, initialThing):
        result = size*[initialThing]
        for i in range(size):
            result[i] = initialThing
        return result

    def __init__(self, pixels, animationTime, firstLED, lastLED):
        Animation.__init__(self, pixels, animationTime, firstLED, lastLED)
        self.LEDArray = []
        self.LEDArray = self.doAllocate(len(self.pixels), False)
        self.TwinkleIndices = self.doAllocate(len(self.LEDArray) // 10, 0)
        self.TwinkleState = 'None'
        self.index = 0
        self.changedCount = 0
        self.twinkleToOn = False
        self.LongestTimeToWait = 0

    def printSelf(self):
        print("Twinkle: ", self.animationStepIncrement)

    def commonTwinkleInitiate(self):
        self.LongestTimeToWait = time.monotonic()+self.animationTime
        self.index = 0
        self.changedCount = 0

    def indexInIndices(self, newidx):
        for i in range(0, len(self.TwinkleIndices)):
            if self.TwinkleIndices[i] == newidx:
                return True
        return False

    def twinkleSomeInit(self):
        for i in range(len(self.TwinkleIndices)):
            self.TwinkleIndices[i] = random.randrange(self.firstLED, self.lastLED)
        self.TwinkleState = 'Twinkling'
        self.twinkleToOn = False
        self.index = 0

    def twinkleSomeLEDs(self, now):
        if now-self.lastUpdateTime < self.animationStepIncrement*2:
            return
        aColor = BLACK
        if self.twinkleToOn:
            aColor = self.randomColor()
        self.pixels[self.TwinkleIndices[self.index]] = aColor
        self.pixels.show()
        if self.twinkleToOn:
            newIdx = self.TwinkleIndices[self.index]
            while self.indexInIndices(newIdx):
                newIdx = random.randrange(self.firstLED, self.lastLED)
            self.TwinkleIndices[self.index] = newIdx
        self.index += 1
        if self.index >= len(self.TwinkleIndices):
            self.index = 0
            self.twinkleToOn = not self.twinkleToOn
        self.lastUpdateTime = now

    def twinkleFinish(self, desiredState):
        if not desiredState:
            self.setAllPixelsTo(BLACK)
            self.pixels.show()
            return
        for i in range(self.firstLED, self.lastLED+1):
            if self.LEDArray[i] != desiredState:
                aColor = self.randomColor()
                if not desiredState:
                    aColor = BLACK
                self.pixels[i] = aColor
        self.pixels.show()

    def Start(self, topToBottom, colorToUse):
        self.topToBottom = topToBottom
        self.colorToUse = colorToUse      # not used here
        for i in range(self.firstLED, self.lastLED+1):
            self.LEDArray[i] = False
        self.active = True
        self.TwinkleState = 'TurningOn'
        self.commonTwinkleInitiate()

    def Continue(self):
        if not self.active:
            return
        now = time.monotonic()
        if now-self.lastUpdateTime < self.animationStepIncrement:
            return
        if self.TwinkleState == 'Twinkling':
            self.twinkleSomeLEDs(now)
            return
        if self.changedCount >= (self.lastLED-self.firstLED) or now > self.LongestTimeToWait:
            self.twinkleFinish(self.TwinkleState == 'TurningOn')
            if self.TwinkleState == 'TurningOn':
                self.TwinkleState = 'Twinkling'
                self.twinkleSomeInit()
            else:
                self.twinkleFinish(False)
                self.active = False
            return
        tryIdx = random.randrange(self.firstLED, self.lastLED)
        desiredState = self.TwinkleState == 'TurningOn'
        color = self.randomColor()
        if self.LEDArray[tryIdx] == desiredState:
            origTryIdx = tryIdx
            while self.LEDArray[tryIdx] == desiredState and tryIdx != origTryIdx:
                tryIdx += 1
                if tryIdx > self.lastLED:
                    tryIdx = self.firstLED
        if self.TwinkleState == 'TurningOff':
            color = BLACK
        self.LEDArray[tryIdx] = desiredState
        self.changedCount += 1
        self.pixels[tryIdx] = color
        self.pixels.show()
        self.lastUpdateTime = now

    def Finish(self, topToBottom):
        self.topToBottom = topToBottom  # unused in Twinkle
        self.commonTwinkleInitiate()
        self.TwinkleState = 'TurningOff'

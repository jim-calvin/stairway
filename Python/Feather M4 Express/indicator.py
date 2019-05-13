import time
import digitalio
import board

class indicator(object):

    DEBUG = False
    # types could be LED, NeoPixel, DotStar

    def __init__(self, pixels, indicatorIdx, type='NeoPixel'):
        self.px = pixels
        self.type = type
        self.blinker = False
        self.index = indicatorIdx
        self.onTime = 0
        self.active = False
        self.isOn = False
        self.color = (0, 0, 0)
        self.aPin = 0
        if self.type == 'LED':
            self.aPin = digitalio.DigitalInOut(board.D13)
            self.aPin.direction = digitalio.Direction.OUTPUT

    def setIndicator(self, toColor):
        now = time.monotonic()
        self.isOn = True
        self.active = True
        if self.type == 'LED':
            self.aPin.value = True
            self.onTime = now
            return
        self.color = toColor
        self.px[self.index] = toColor
        if (toColor == (0, 0, 0)):
            self.active = False
            return
        self.onTime = now

    def Continue(self):
        if not self.active:
            return
        if self.blinker:
            if time.monotonic()-self.onTime < 0.25:
                return
            if self.type == "LED":
                self.aPin.value = self.isOn
            elif not self.isOn:
                self.px[self.index] = self.color
            else:
                self.px[self.index] = (0, 0, 0)
            self.isOn = not self.isOn
            self.onTime = time.monotonic()
        if time.monotonic()-self.onTime < 30:
            return
        self.isOn = False
        self.px[self.index] = (0, 0, 0)

class blinker(indicator):

    def __init__(self, pixels, indicatorIdx, type='NeoPixel'):
        indicator.__init__(self, pixels, indicatorIdx, type)
        self.blinker = True

    def Continue(self):
        now = time.monotonic()
        if not self.active:
            return
        if now-self.onTime < 0.25:
            return
        if self.type == 'LED':
            self.aPin.value = self.isOn
        elif not self.isOn:
            self.px[self.index] = self.color
        else:
            self.px[self.index] = (0, 0, 0)
        self.isOn = not self.isOn
        self.onTime = now
import digitalio
import time

class PIR(object):

    DEBUG = False
    FAKEPIR = False

    def __init__(self, pin_number, pixels, indicatorIdx, indicatorColor, name):
        self.aPin = digitalio.DigitalInOut(pin_number)
        self.aPin.direction = digitalio.Direction.INPUT
        self.aPin.pull = digitalio.Pull.UP
        self.pixels = pixels
        self.indicatorIdx = indicatorIdx
        self.reportedState = False
        self.previousState = False
        self.transitionTime = 0
        self.indicatorColor = indicatorColor
        self.name = name

    def read(self):
        state = self.aPin.value
        if self.FAKEPIR:
            state = not state      # for bench top debug
        now = time.monotonic()
        if (state != self.previousState):
            color = 0
            if (state != 0):
                color = self.indicatorColor
            self.pixels[self.indicatorIdx] = color
            self.pixels.show()
            self.tranistionTime = now
            self.previousState = state
        if (now - self.transitionTime) > 100:
            self.tranistionTime = now
            self.reportedState = state
        return self.reportedState
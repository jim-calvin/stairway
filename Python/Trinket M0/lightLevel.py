from analogio import AnalogIn
import time

class lightLevel(object):

    DEBUG = False

    def __init__(self, pin_number):
        self.lightLevel = AnalogIn(pin_number)
        self.lastLevel = 0
        self.minRecorded = 65535
        self.maxRecorded = 0
        self.samplingStartTime = 0
        self.lightLevelBright = 56054
        self.lightLevelMedium = 32031
        self.lightLevelDim = 14414
        self.firstDay = True
        self.lightLevelSamplingStartTime = time.monotonic()
        self.lastLightReadTime = 0

    def read(self, LEDsActive):
        if LEDsActive:
            return self.lastLevel
        self.lastLevel = self.lightLevel.value
        if self.lastLevel > self.maxRecorded:
            self.maxRecorded = self.lastLevel
        if self.lastLevel < self.minRecorded:
            self.minRecorded = self.lastLevel
        return self.lastLevel

    def myMap(self, x, inMin, inMax, outMin, outMax):
        return (x-inMin)*(outMax - outMin)/(inMax - outMin) + outMin

    def remapLightLevelThresholds(self):
        self.lightLevelBright = self.myMap(56054, 0, 65535, self.minRecorded,
                                           self.maxRecorded)
        self.lightLevelMedium = self.myMap(32031, 0, 65535, self.minRecorded,
                                           self.maxRecorded)
        self.lightLevelDim = self.myMap(14414, 0, 65535, self.minRecorded,
                                        self.maxRecorded)

    def getLightLevelThresholds(self):
        return (self.lightLevelBright, self.lightLevelMedium,
                self.lightLevelDim)

    def processLightLevel(self, now, LEDsActive):
        if self.firstDay and (now-self.lightLevelSamplingStartTime) > 60*30:
            self.remapLightLevelThresholds()
            self.lightLevelSamplingStartTime = now
        if (now-self.lightLevelSamplingStartTime) > 60*60*24:
            self.remapLightLevelThresholds()
            self.lightLevelSamplingStartTime = time.monotonic()
            self.lightLevelMin = 65535
            self.lightLevelMax = 0
            self.firstDay = False
        if (now-self.lastLightReadTime) > 10:
            self.read(LEDsActive)
            self.lastLightReadTime = now
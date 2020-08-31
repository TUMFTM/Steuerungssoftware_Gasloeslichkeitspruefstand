#pragma once

#include <Arduino.h>

class MachineState;
class CmdMessenger;
class CanMessage;
class Adafruit_ADS1115;
class SimpleTimer;
class RTC_PCF8523;
class SdFat;

class Controller
{
    public:
        static int conversionFactor;
        static MachineState *actualMachineState;
        static CmdMessenger cmdMessenger;
        static CanMessage canMessageCtrl;
        static Adafruit_ADS1115 analogDigitalConverter;
        static SimpleTimer timer;
        static RTC_PCF8523 rtc;
        static SdFat SD;
        void init();
        void doLoop();
        static void readConfigurationFromSDCard();
};

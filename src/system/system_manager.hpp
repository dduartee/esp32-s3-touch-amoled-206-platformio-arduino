#pragma once

#include <Arduino.h>

#include <HWCDC.h>
#include <Wire.h>
#include <LittleFS.h>

#include "config.h"
#include "pmu/pmu.hpp"
#include "display/display.hpp"
#include "button/button.hpp"
#include "storage/fs_manager.hpp"

class SystemManager {
private:
    bool initialized = false;
    bool sleeping = false;
    HWCDC* usbSerial;
    TwoWire* i2c = nullptr;
    PMU pmu;
    FSManager fsManager;
    Display display;
    //TouchController touch;

    void sleep();
    void wakeup();
    void logHeartbeat();
public:
    SystemManager() : usbSerial(nullptr), i2c(nullptr) {}
    bool init(HWCDC &USBSerial);
    void update();
    PMU& getPMU() { return pmu; }
    Display& getDisplay() { return display; }
    HWCDC* getUSBSerial() { return usbSerial; }
    TwoWire* getI2C() { return i2c; }
};
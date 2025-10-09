#pragma once

#include <Arduino.h>

#include <HWCDC.h>
#include <Wire.h>

#include "config.h"
#include "pmu/pmu.hpp"
#include "display/display.hpp"
#include "storage/esp32_flash_storage.hpp"
#include "storage/flash_file_system.hpp"

class SystemManager {
private:
    bool initialized = false;
    HWCDC* usbSerial;
    TwoWire* i2c = nullptr;
    PMU pmu;
    Display display;
    ESP32FlashStorage flashStorage;
    FlashFileSystem fileSystem;
    //TouchController touch;

    void logHeartbeat();
public:
    SystemManager() : usbSerial(nullptr), i2c(nullptr) {}
    bool init(HWCDC &USBSerial);
    void update();
    PMU& getPMU() { return pmu; }
    Display& getDisplay() { return display; }
    ESP32FlashStorage& getFlashStorage() { return flashStorage; }
    FlashFileSystem& getFileSystem() { return fileSystem; }
    HWCDC* getUSBSerial() { return usbSerial; }
    TwoWire* getI2C() { return i2c; }
};
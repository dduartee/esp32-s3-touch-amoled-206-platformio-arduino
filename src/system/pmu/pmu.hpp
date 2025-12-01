#pragma once
#include "config.h"

#include "XPowersAXP2101.tpp"

#include "../../logger/logger.hpp"

class PMU {
private:
    Logger* logger = nullptr;
    XPowersAXP2101 pmu;
    uint8_t pmuAddress = 0x34;
    bool initialized = false;
public:
    PMU(Logger *logger) { this->logger = logger; };
    bool setBus(TwoWire &wire);
    
    bool isInitialized() const { return initialized; }
    
    bool isBatteryConnect() {
        return initialized ? pmu.isBatteryConnect() : false;
    }
    
    bool isCharging() {
        return initialized ? pmu.isCharging() : false;
    }
    
    bool isUSBConnected() {
        return initialized ? pmu.isVbusIn() : false;
    }
    
    uint8_t getBatteryPercent() {
        return initialized ? pmu.getBatteryPercent() : 0;
    }
    
    uint16_t getBattVoltage() {
        return initialized ? pmu.getBattVoltage() : 0;
    }
};
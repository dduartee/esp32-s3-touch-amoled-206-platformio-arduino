#include "pmu.hpp"
#include "../../logger.hpp"

bool PMU::init(HWCDC &serial, TwoWire &wire) {
    logger.debug("PMU", "Starting AXP2101 initialization...");
    
    if(!this->pmu.begin(wire, this->pmuAddress, PMU_SDA, PMU_SCL)) {
        logger.failure("PMU", "AXP2101 not found");
        initialized = false;
        return false;
    }
    
    logger.success("PMU", "AXP2101 initialized successfully");
    initialized = true;
    return true;
}

bool PMU::isBatteryConnect() {
    return this->initialized ? this->pmu.isBatteryConnect() : false;
}

bool PMU::isCharging() {
    return this->initialized ? this->pmu.isCharging() : false;
}

bool PMU::isUSBConnected() {
    return this->initialized ? this->pmu.isVbusIn() : false;
}

uint8_t PMU::getBatteryPercent() {
    return this->initialized ? this->pmu.getBatteryPercent() : 0;
}

uint16_t PMU::getBattVoltage() {
    return this->initialized ? this->pmu.getBattVoltage() : 0;
}
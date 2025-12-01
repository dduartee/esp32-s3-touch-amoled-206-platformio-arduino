#pragma once
#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "../../logger/logger.hpp"

class IMU {
private:
    static constexpr uint8_t ADDR_QMI8658 = 0x6B;
    static constexpr uint8_t CHIP_ID = 0x05;
    
    TwoWire* i2c = nullptr;
    Logger* logger = nullptr;
    bool initialized = false;
    
    // QMI8658 Register addresses
    enum Registers : uint8_t {
        REG_WHO_AM_I = 0x00,
        REG_REVISION_ID = 0x01,
        REG_CTRL1 = 0x02,
        REG_CTRL2 = 0x03,
        REG_CTRL3 = 0x04,
        REG_CTRL4 = 0x05,
        REG_CTRL5 = 0x06,
        REG_CTRL7 = 0x08,
        REG_CTRL8 = 0x09,
        REG_STATUSINT = 0x2D,
        REG_STATUS0 = 0x2E,
        REG_STATUS1 = 0x2F,
        REG_TIMESTAMP_L = 0x30,
        REG_TEMP_L = 0x33,
        REG_TEMP_H = 0x34,
        REG_AX_L = 0x35,
        REG_AX_H = 0x36,
        REG_AY_L = 0x37,
        REG_AY_H = 0x38,
        REG_AZ_L = 0x39,
        REG_AZ_H = 0x3A,
        REG_GX_L = 0x3B,
        REG_GX_H = 0x3C,
        REG_GY_L = 0x3D,
        REG_GY_H = 0x3E,
        REG_GZ_L = 0x3F,
        REG_GZ_H = 0x40,
        REG_RESET = 0x60,
    };
    
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t* value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, size_t len);

public:
    struct AccelData {
        float x;  // g
        float y;  // g
        float z;  // g
    };
    
    struct GyroData {
        float x;  // dps (degrees per second)
        float y;  // dps
        float z;  // dps
    };
    
    IMU(Logger* logger) : logger(logger) {}
    
    bool setBus(TwoWire &bus);
    bool isInitialized() const { return initialized; }
    
    bool readAccel(AccelData& data);
    bool readGyro(GyroData& data);
    bool readTemperature(float& temp);
};

#include "imu.hpp"

bool IMU::setBus(TwoWire &bus) {
    i2c = &bus;
    
    // Read chip ID
    uint8_t whoami = 0;
    if (!readRegister(REG_WHO_AM_I, &whoami)) {
        if (logger) logger->failure("IMU", "QMI8658 not found");
        return false;
    }
    
    if (logger) logger->info("IMU", (String("Chip ID: 0x") + String(whoami, HEX)).c_str());
    
    // Software reset
    writeRegister(REG_RESET, 0xB0);
    delay(10);
    
    // Configure CTRL1: Set serial interface and address auto increment
    if (!writeRegister(REG_CTRL1, 0x40)) {
        if (logger) logger->failure("IMU", "Failed to configure CTRL1");
        return false;
    }
    
    // Configure accelerometer: 8g range, 128Hz ODR
    // CTRL2: [7:4] = accel range (0011 = 8g), [3:0] = ODR (0110 = 128Hz)
    if (!writeRegister(REG_CTRL2, 0x36)) {
        if (logger) logger->failure("IMU", "Failed to configure accelerometer");
        return false;
    }
    
    // Configure gyroscope: 1024dps range, 128Hz ODR
    // CTRL3: [7:4] = gyro range (0110 = 1024dps), [3:0] = ODR (0110 = 128Hz)
    if (!writeRegister(REG_CTRL3, 0x66)) {
        if (logger) logger->failure("IMU", "Failed to configure gyroscope");
        return false;
    }
    
    // Enable accelerometer and gyroscope
    // CTRL7: [7] = enable gyro, [6] = enable accel
    if (!writeRegister(REG_CTRL7, 0x03)) {
        if (logger) logger->failure("IMU", "Failed to enable sensors");
        return false;
    }
    
    delay(50);
    
    if (logger) logger->success("IMU", "QMI8658 initialized");
    initialized = true;
    return true;
}

bool IMU::writeRegister(uint8_t reg, uint8_t value) {
    if (!i2c) return false;
    
    i2c->beginTransmission(ADDR_QMI8658);
    i2c->write(reg);
    i2c->write(value);
    return (i2c->endTransmission() == 0);
}

bool IMU::readRegister(uint8_t reg, uint8_t* value) {
    return readRegisters(reg, value, 1);
}

bool IMU::readRegisters(uint8_t reg, uint8_t* buffer, size_t len) {
    if (!i2c) return false;
    
    i2c->beginTransmission(ADDR_QMI8658);
    i2c->write(reg);
    if (i2c->endTransmission(false) != 0) return false;
    
    if (i2c->requestFrom(ADDR_QMI8658, len) != len) return false;
    
    for (size_t i = 0; i < len; i++) {
        buffer[i] = i2c->read();
    }
    
    return true;
}

bool IMU::readAccel(AccelData& data) {
    if (!initialized) return false;
    
    uint8_t raw[6];
    if (!readRegisters(REG_AX_L, raw, 6)) return false;
    
    // Combine bytes (little endian)
    int16_t ax = (int16_t)(raw[1] << 8 | raw[0]);
    int16_t ay = (int16_t)(raw[3] << 8 | raw[2]);
    int16_t az = (int16_t)(raw[5] << 8 | raw[4]);
    
    // Convert to g (8g range, 16-bit)
    const float scale = 8.0f / 32768.0f;
    data.x = ax * scale;
    data.y = ay * scale;
    data.z = az * scale;
    
    return true;
}

bool IMU::readGyro(GyroData& data) {
    if (!initialized) return false;
    
    uint8_t raw[6];
    if (!readRegisters(REG_GX_L, raw, 6)) return false;
    
    // Combine bytes (little endian)
    int16_t gx = (int16_t)(raw[1] << 8 | raw[0]);
    int16_t gy = (int16_t)(raw[3] << 8 | raw[2]);
    int16_t gz = (int16_t)(raw[5] << 8 | raw[4]);
    
    // Convert to dps (1024dps range, 16-bit)
    const float scale = 1024.0f / 32768.0f;
    data.x = gx * scale;
    data.y = gy * scale;
    data.z = gz * scale;
    
    return true;
}

bool IMU::readTemperature(float& temp) {
    if (!initialized) return false;
    
    uint8_t raw[2];
    if (!readRegisters(REG_TEMP_L, raw, 2)) return false;
    
    int16_t temp_raw = (int16_t)(raw[1] << 8 | raw[0]);
    
    // Convert to celsius (datasheet formula)
    temp = temp_raw / 256.0f;
    
    return true;
}

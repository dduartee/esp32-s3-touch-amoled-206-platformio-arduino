#include "rtc.hpp"

bool RTC::setBus(TwoWire &bus) {
    i2c = &bus;
    
    // Test communication by reading control register
    uint8_t ctrl1 = 0;
    if (!readRegister(REG_CONTROL_1, &ctrl1)) {
        if (logger) logger->failure("RTC", "PCF85063 not found");
        return false;
    }
    
    // Enable RTC, disable 12h mode (use 24h)
    if (!writeRegister(REG_CONTROL_1, 0x00)) {
        if (logger) logger->failure("RTC", "Failed to configure PCF85063");
        return false;
    }
    
    // Setup interrupt pin
    pinMode(interrupt_pin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(interrupt_pin), RTC::isrArg, this, FALLING);
    
    if (logger) logger->success("RTC", "PCF85063 initialized");
    initialized = true;
    return true;
}

void IRAM_ATTR RTC::isrArg(void* arg) {
    RTC* self = static_cast<RTC*>(arg);
    if (self) {
        self->alarm_triggered = true;
    }
}

bool RTC::writeRegister(uint8_t reg, uint8_t value) {
    if (!i2c) return false;
    
    i2c->beginTransmission(ADDR_PCF85063);
    i2c->write(reg);
    i2c->write(value);
    return (i2c->endTransmission() == 0);
}

bool RTC::readRegister(uint8_t reg, uint8_t* value) {
    return readRegisters(reg, value, 1);
}

bool RTC::readRegisters(uint8_t reg, uint8_t* buffer, size_t len) {
    if (!i2c) return false;
    
    i2c->beginTransmission(ADDR_PCF85063);
    i2c->write(reg);
    if (i2c->endTransmission(false) != 0) return false;
    
    if (i2c->requestFrom(ADDR_PCF85063, len) != len) return false;
    
    for (size_t i = 0; i < len; i++) {
        buffer[i] = i2c->read();
    }
    
    return true;
}

bool RTC::setDateTime(const DateTime& dt) {
    if (!initialized) return false;
    
    uint8_t data[7];
    data[0] = decToBcd(dt.second) & 0x7F;  // Clear OS flag
    data[1] = decToBcd(dt.minute);
    data[2] = decToBcd(dt.hour);
    data[3] = decToBcd(dt.day);
    data[4] = dt.weekday & 0x07;
    data[5] = decToBcd(dt.month);
    data[6] = decToBcd(dt.year - 2000);
    
    // Write all time/date registers at once
    i2c->beginTransmission(ADDR_PCF85063);
    i2c->write(REG_SECONDS);
    for (int i = 0; i < 7; i++) {
        i2c->write(data[i]);
    }
    
    bool success = (i2c->endTransmission() == 0);
    
    if (logger) {
        if (success) {
            logger->success("RTC", "Date/Time set");
        } else {
            logger->failure("RTC", "Failed to set Date/Time");
        }
    }
    
    return success;
}

bool RTC::getDateTime(DateTime& dt) {
    if (!initialized) return false;
    
    uint8_t data[7];
    if (!readRegisters(REG_SECONDS, data, 7)) return false;
    
    dt.second = bcdToDec(data[0] & 0x7F);
    dt.minute = bcdToDec(data[1] & 0x7F);
    dt.hour = bcdToDec(data[2] & 0x3F);
    dt.day = bcdToDec(data[3] & 0x3F);
    dt.weekday = data[4] & 0x07;
    dt.month = bcdToDec(data[5] & 0x1F);
    dt.year = bcdToDec(data[6]) + 2000;
    
    return true;
}

bool RTC::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    DateTime dt;
    if (!getDateTime(dt)) return false;
    
    dt.hour = hour;
    dt.minute = minute;
    dt.second = second;
    
    return setDateTime(dt);
}

bool RTC::setDate(uint16_t year, uint8_t month, uint8_t day) {
    DateTime dt;
    if (!getDateTime(dt)) return false;
    
    dt.year = year;
    dt.month = month;
    dt.day = day;
    
    return setDateTime(dt);
}

bool RTC::setAlarm(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day) {
    if (!initialized) return false;
    
    // 0x80 = alarm disabled for that field
    uint8_t alarm_second = (second == 0xFF) ? 0x80 : (decToBcd(second) & 0x7F);
    uint8_t alarm_minute = (minute == 0xFF) ? 0x80 : (decToBcd(minute) & 0x7F);
    uint8_t alarm_hour = (hour == 0xFF) ? 0x80 : (decToBcd(hour) & 0x3F);
    uint8_t alarm_day = (day == 0xFF) ? 0x80 : (decToBcd(day) & 0x3F);
    uint8_t alarm_weekday = 0x80; // Disable weekday alarm
    
    // Write alarm registers
    if (!writeRegister(REG_SECOND_ALARM, alarm_second)) return false;
    if (!writeRegister(REG_MINUTE_ALARM, alarm_minute)) return false;
    if (!writeRegister(REG_HOUR_ALARM, alarm_hour)) return false;
    if (!writeRegister(REG_DAY_ALARM, alarm_day)) return false;
    if (!writeRegister(REG_WEEKDAY_ALARM, alarm_weekday)) return false;
    
    // Enable alarm interrupt in CONTROL_2 (bit 7 = AIE)
    uint8_t ctrl2 = 0;
    if (!readRegister(REG_CONTROL_2, &ctrl2)) return false;
    ctrl2 |= 0x80; // Set AIE bit
    if (!writeRegister(REG_CONTROL_2, ctrl2)) return false;
    
    if (logger) {
        logger->success("RTC", (String("Alarm set: ") + String(hour) + ":" + String(minute)).c_str());
    }
    
    return true;
}

bool RTC::clearAlarm() {
    if (!initialized) return false;
    
    // Disable alarm interrupt in CONTROL_2
    uint8_t ctrl2 = 0;
    if (!readRegister(REG_CONTROL_2, &ctrl2)) return false;
    ctrl2 &= ~0x80; // Clear AIE bit
    ctrl2 &= ~0x40; // Clear AF (alarm flag)
    if (!writeRegister(REG_CONTROL_2, ctrl2)) return false;
    
    alarm_triggered = false;
    
    if (logger) logger->info("RTC", "Alarm cleared");
    
    return true;
}

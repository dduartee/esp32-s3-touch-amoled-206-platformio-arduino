#pragma once
#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "../../logger/logger.hpp"

class RTC {
private:
    static constexpr uint8_t ADDR_PCF85063 = 0x51;
    
    TwoWire* i2c = nullptr;
    Logger* logger = nullptr;
    bool initialized = false;
    
    // PCF85063 Register addresses
    enum Registers : uint8_t {
        REG_CONTROL_1 = 0x00,
        REG_CONTROL_2 = 0x01,
        REG_OFFSET = 0x02,
        REG_RAM_BYTE = 0x03,
        REG_SECONDS = 0x04,
        REG_MINUTES = 0x05,
        REG_HOURS = 0x06,
        REG_DAYS = 0x07,
        REG_WEEKDAYS = 0x08,
        REG_MONTHS = 0x09,
        REG_YEARS = 0x0A,
        REG_SECOND_ALARM = 0x0B,
        REG_MINUTE_ALARM = 0x0C,
        REG_HOUR_ALARM = 0x0D,
        REG_DAY_ALARM = 0x0E,
        REG_WEEKDAY_ALARM = 0x0F,
    };
    
    uint8_t interrupt_pin = RTC_INT;
    volatile bool alarm_triggered = false;
    
    // Helper functions
    uint8_t bcdToDec(uint8_t val) { return (val / 16 * 10) + (val % 16); }
    uint8_t decToBcd(uint8_t val) { return (val / 10 * 16) + (val % 10); }
    
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t* value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, size_t len);
    static void IRAM_ATTR isrArg(void* arg);

public:
    struct DateTime {
        uint8_t second;
        uint8_t minute;
        uint8_t hour;
        uint8_t day;
        uint8_t weekday;
        uint8_t month;
        uint16_t year;
    };
    
    RTC(Logger* logger) : logger(logger) {}
    
    bool setBus(TwoWire &bus);
    bool isInitialized() const { return initialized; }
    
    bool setDateTime(const DateTime& dt);
    bool getDateTime(DateTime& dt);
    
    // Convenience functions
    bool setTime(uint8_t hour, uint8_t minute, uint8_t second);
    bool setDate(uint16_t year, uint8_t month, uint8_t day);
    
    // Alarm functions
    bool setAlarm(uint8_t hour, uint8_t minute, uint8_t second = 0xFF, uint8_t day = 0xFF);
    bool clearAlarm();
    bool isAlarmTriggered() { return alarm_triggered; }
    void clearAlarmFlag() { alarm_triggered = false; }
};

#pragma once
#include <Arduino.h>
#include <HWCDC.h>

/**
 * Simple logging system for ESP32-S3 Touch AMOLED project
 * Provides consistent formatting and log levels
 */
class Logger {
private:
    HWCDC* serial = nullptr;
    bool initialized = false;
    
public:
    enum Level {
        ERROR = 0,
        WARN = 1, 
        INFO = 2,
        DEBUG = 3
    };
    
    Logger(){};

    void setSerial(HWCDC* serial) {
        this->serial = serial;
        this->initialized = (serial != nullptr);
    }

    // Generic print to generalize messages
    void print(const char* level, const char* component, const char* message);
    
    // Error messages (always shown)
    void error(const char* component, const char* message);
    
    // Warning messages
    void warn(const char* component, const char* message);
    
    // Info messages (normal operation)
    void info(const char* component, const char* message);
    
    // Debug messages (detailed info)
    void debug(const char* component, const char* message);
    
    // Success messages
    void success(const char* component, const char* message);
    
    // Failure messages
    void failure(const char* component, const char* message);
    
    // Section headers
    void header(const char* title);
    
    // Section footers
    void footer();
    
    // Raw println (for compatibility)
    void println(const char* message);
};

// Global logger instance
extern Logger logger;
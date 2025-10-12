#pragma once
#include <HWCDC.h>

/**
 * Simple logging system for ESP32-S3 Touch AMOLED project
 * Provides consistent formatting and log levels
 */
class Logger {
private:
    HWCDC* serial;
    bool enabled;
    
public:
    enum Level {
        ERROR = 0,
        WARN = 1, 
        INFO = 2,
        DEBUG = 3
    };
    
    Logger() : serial(nullptr), enabled(false) {}
    
    void init(HWCDC* serialPtr) {
        serial = serialPtr;
        enabled = (serialPtr != nullptr);
    }

    // Generic print to generalize messages
    void print(const char* level, const char* component, const char* message) {
        if (!enabled) return;
        serial->print("[");
        serial->print(level);
        serial->print("] ");
        serial->print(component);
        serial->print(": ");
        serial->println(message);
    }
    
    // Error messages (always shown)
    void error(const char* component, const char* message) {
        if (!enabled) return;
        print("ERROR", component, message);
    }
    
    // Warning messages
    void warn(const char* component, const char* message) {
        if (!enabled) return;
        print("WARN", component, message);
    }
    
    // Info messages (normal operation)
    void info(const char* component, const char* message) {
        if (!enabled) return;
        print("INFO", component, message);
    }
    
    // Debug messages (detailed info)
    void debug(const char* component, const char* message) {
        if (!enabled) return;
        print("DEBUG", component, message);
    }
    
    // Success messages
    void success(const char* component, const char* message) {
        if (!enabled) return;
        print("SUCCESS", component, message);
    }
    
    // Failure messages
    void failure(const char* component, const char* message) {
        if (!enabled) return;
        print("FAILURE", component, message);
    }
    
    // Section headers
    void header(const char* title) {
        if (!enabled) return;
        serial->println("==========================================");
        serial->print("| ");
        serial->println(title);
        serial->println("==========================================");
    }
    
    // Section footers
    void footer() {
        if (!enabled) return;
        serial->println("==========================================");
    }
    
    // Raw println (for compatibility)
    void println(const char* message) {
        if (!enabled) return;
        serial->println(message);
    }
};

// Global logger instance
extern Logger logger;
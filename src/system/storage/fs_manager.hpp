#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "logger/logger.hpp"

class FSManager {
private:
    Logger* logger = nullptr;
    bool initialized = false;
public:
    FSManager(Logger* logger);
    ~FSManager();

    bool isInitialized() const { return initialized; }
    size_t totalKB() { return LittleFS.totalBytes() / 1024; }
    size_t usedKB() { return LittleFS.usedBytes() / 1024; }
    bool exists(const char* path) { return LittleFS.exists(path); }
    bool writeFile(const char* path, const String& data);
    String readFile(const char* path);
};

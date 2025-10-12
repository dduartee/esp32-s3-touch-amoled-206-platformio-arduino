#include <Arduino.h>
#include "system/system_manager.hpp"
#include "config.h"
#include "logger/logger.hpp"

HWCDC USBSerial;
Logger logger = Logger();
SystemManager *system_manager = nullptr;

void setup() {
    // Initialize USB Serial
    USBSerial.begin(115200);
    while (!USBSerial) {
        ; // Wait for serial port to connect. Needed for native USB
    }
    
    // Initialize logger
    logger.setSerial(&USBSerial);

    // Welcome message
    logger.header("ESP32-S3 Touch AMOLED System Setup");
    logger.info("MAIN", "System starting...");
    logger.info("MAIN", "Version: 1.0.0");

    // Initialize system
    system_manager = new SystemManager(&logger);

    if (!system_manager->isInitialized()) {
        logger.error("MAIN", "System initialization failed - halting");
        while(true) { 
            delay(1000);
            logger.error("MAIN", "System halted due to initialization failure");
        }
    }
    
    logger.header("System setup complete");
}

void loop() {
    // Main loop code
    system_manager->update();
}

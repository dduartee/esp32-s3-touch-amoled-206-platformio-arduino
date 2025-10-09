#include <Arduino.h>
#include "HWCDC.h"
#include "system/system_manager.hpp"
#include "config.h"
#include "logger.hpp"

HWCDC USBSerial;
SystemManager system_manager;

void setup() {
    // Initialize USB Serial
    USBSerial.begin(115200);
    while (!USBSerial) {
        ; // Wait for serial port to connect. Needed for native USB
    }
    
    // Initialize logger
    logger.init(&USBSerial);
    
    // Welcome message
    logger.header("ESP32-S3 Touch AMOLED System Setup");
    logger.info("MAIN", "System starting...");
    logger.info("MAIN", "Version: 1.0.0");
    
    // Initialize system
    if (!system_manager.init(USBSerial)) {
        logger.error("MAIN", "System initialization failed - halting");
        while(true) { 
            delay(1000);
            logger.error("MAIN", "System halted due to initialization failure");
        }
    }
    
    // Test Flash Storage
    logger.header("Testing 24MB Flash Storage");
    auto& flash = system_manager.getFlashStorage();
    
    if (flash.runTest()) {
        logger.success("TEST", "Flash storage test passed!");
        
        // Show flash info
        logger.info("FLASH_INFO", flash.getFlashInfo().c_str());
        
        // Test user operations
        logger.info("TEST", "Writing sample data...");
        flash.writeString(0x0000, "Hello from 24MB Flash!");
        flash.writeString(0x1000, "ESP32-S3 AMOLED Touch Display");
        
        String text1 = flash.readString(0x0000);
        String text2 = flash.readString(0x1000);
        
        logger.success("TEST", String("Read back: '" + text1 + "'").c_str());
        logger.success("TEST", String("Read back: '" + text2 + "'").c_str());
    } else {
        logger.failure("TEST", "Flash storage test failed!");
    }
    
    logger.header("System setup complete");
}

void loop() {
    // Main loop code
    system_manager.update();
}

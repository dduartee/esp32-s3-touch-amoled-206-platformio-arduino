#include "system_manager.hpp"
#include "../logger.hpp"

bool SystemManager::init(HWCDC &usbSerial) {
    // Store pointers to passed objects
    this->usbSerial = &usbSerial;
    
    // Initialize logger
    logger.init(&usbSerial);
    
    logger.header("SystemManager Initialization");
    
    // Initialize I2C bus (400kHz Fast Mode)
    logger.info("I2C", "Initializing bus at 400kHz...");
    Wire.begin(I2C_SDA, I2C_SCL, 400000); 
    this->i2c = &Wire;
    logger.success("I2C", "Bus initialized at 400kHz");
    
    // Initialize PMU
    logger.info("PMU", "Initializing AXP2101...");
    if (!pmu.init(usbSerial, Wire)) {
        logger.failure("PMU", "AXP2101 initialization failed");
        logger.footer();
        return false;
    }

    // Initialize Display
    logger.info("DISPLAY", "Initializing CO5300 AMOLED...");
    if (!display.init(usbSerial)) {
        logger.failure("DISPLAY", "CO5300 initialization failed");
        logger.footer();
        return false;
    }

    // Try to mount LittleFS first without formatting
    if(!LittleFS.begin(false)) {
        logger.info("LittleFS", "Initial mount failed, attempting format...");
        
        // If mount fails, format and try again
        if(!LittleFS.begin(true)) {
            logger.failure("LittleFS", "Failed to mount LittleFS even after formatting");
            logger.footer();
            return false;
        }
        logger.success("LittleFS", "LittleFS formatted and mounted successfully");
    } else {
        logger.success("LittleFS", "LittleFS mounted successfully");
    }

    logger.info("LittleFS", String("Total space: " + String(LittleFS.totalBytes() / 1024) + " KB").c_str());
    logger.info("LittleFS", String("Used space: " + String(LittleFS.usedBytes() / 1024) + " KB").c_str());

    // Create test file if it doesn't exist
    if (!LittleFS.exists("/test.txt")) {
        File testFile = LittleFS.open("/test.txt", FILE_WRITE);
        if (testFile) {
            testFile.print("Hello ESP32S3 LittleFS Storage");
            testFile.close();
            logger.info("LittleFS", "Created test file");
        }
    }

    File file = LittleFS.open("/test.txt");
    if(!file || file.isDirectory()){
        logger.failure("LittleFS", "Failed to open test file for reading");
        logger.footer();
        return false;
    }

    logger.info("LittleFS", "Contents of /test.txt:");
    while(file.available()){
        String line = file.readStringUntil('\n');
        usbSerial.println(line);
    }
    file.close();
    logger.success("LittleFS", "File read successfully");

    file = LittleFS.open("/config.json", FILE_WRITE);
    file.print("{\"setting\":\"value\"}");
    file.close();

    file = LittleFS.open("/config.json");

    if(!file || file.isDirectory()){
        logger.failure("LittleFS", "Failed to open config file for reading");
        logger.footer();
        return false;
    }

    logger.info("LittleFS", "Contents of /config.json:");
    while(file.available()){
        String line = file.readStringUntil('\n');
        usbSerial.println(line);
    }
    file.close();
    logger.success("LittleFS", "File read successfully");

    logger.success("SYSTEM", "All components initialized successfully");
    logger.footer();
    
    this->initialized = true;
    return true;
}

void SystemManager::update() {
    static unsigned long lastTime = 0;
    
    // Heartbeat every 5 seconds
    if (millis() - lastTime > 5000) {
        lastTime = millis();

        this->logHeartbeat();
    }
}

void SystemManager::logHeartbeat() {
    static unsigned long lastTime = 0;
    static int heartbeat = 0;
    
    // Heartbeat every 5 seconds
    if (millis() - lastTime > 5000) {
        lastTime = millis();
        heartbeat++;

        this->usbSerial->println("========================================");
        this->usbSerial->print("| HEARTBEAT #");
        this->usbSerial->println(heartbeat);
        this->usbSerial->print("| UPTIME: ");
        this->usbSerial->print(millis() / 1000);
        this->usbSerial->println(" seconds");
        
        // Memory Status
        this->usbSerial->print("| Internal RAM Free: ");
        this->usbSerial->print(ESP.getFreeHeap() / 1024);
        this->usbSerial->println(" KB");

        this->usbSerial->print("| PSRAM Free: ");
        this->usbSerial->print(ESP.getFreePsram() / 1024);
        this->usbSerial->println(" KB");

        this->usbSerial->print("| FLASH Size: ");
        this->usbSerial->print(ESP.getFlashChipSize() / 1024);
        this->usbSerial->println(" KB");

        this->usbSerial->print("| Battery Voltage: ");
        this->usbSerial->print(this->getPMU().getBattVoltage());
        this->usbSerial->print(" mV / ");
        this->usbSerial->print(this->getPMU().getBatteryPercent());
        this->usbSerial->println(" %");

        this->usbSerial->print("| USB Connected: ");
        this->usbSerial->println(this->getPMU().isUSBConnected() ? "Yes" : "No");

        this->usbSerial->print("| Battery Connected: ");
        this->usbSerial->println(this->getPMU().isBatteryConnect() ? "Yes" : "No");

        this->usbSerial->print("| Charging: ");
        this->usbSerial->println(this->getPMU().isCharging() ? "Yes" : "No");

        this->usbSerial->println("| USB SERIAL + PSRAM STABLE!");
        this->usbSerial->println("========================================");
    }
}

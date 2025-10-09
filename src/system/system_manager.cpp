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

    // Initialize ESP32 Flash Storage
    logger.info("ESP32_FLASH", "Initializing 32MB Flash Storage...");
    if (!flashStorage.init()) {
        logger.failure("ESP32_FLASH", "Flash storage initialization failed");
        logger.footer();
        return false;
    }

    // Initialize File System
    logger.info("FLASH_FS", "Initializing Flash File System...");
    if (!fileSystem.init(&flashStorage)) {
        logger.failure("FLASH_FS", "File system initialization failed");
        logger.footer();
        return false;
    }

    // Im main.cpp oder wo auch immer:
    auto& fs = getFileSystem();

    // 😎 KEINE Größen-Management mehr nötig!
    fs.writeStringAsFile("config.json", "{\"theme\":\"dark\",\"brightness\":80}");
    fs.writeStringAsFile("wifi.json", "{\"ssid\":\"MyNetwork\",\"password\":\"secret\"}");

    // Lesen ist genauso einfach
    String config = fs.readFileAsString("config.json");
    String wifiConfig = fs.readFileAsString("wifi.json");

    // File Management
    auto files = fs.listFiles();  // Alle Dateien auflisten
    bool exists = fs.exists("logo.png");
    uint32_t size = fs.getFileSize("background.jpg");
    fs.deleteFile("old_file.txt");

    // Statistiken
    auto stats = fs.getStats();
    logger.info("FS", ("Used: " + String(stats.usedSpace / 1024) + "KB").c_str());
    logger.info("FS", ("Free: " + String(stats.freeSpace / 1024) + "KB").c_str());
    logger.info("FS", ("Files: " + String(stats.fileCount)).c_str());

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
        this->usbSerial->print(ESP.getFreeHeap());
        this->usbSerial->println(" bytes");

        this->usbSerial->print("| PSRAM Free: ");
        this->usbSerial->print(ESP.getFreePsram());
        this->usbSerial->println(" bytes");

        this->usbSerial->print("| FLASH Size: ");
        this->usbSerial->print(ESP.getFlashChipSize());
        this->usbSerial->println(" bytes");

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

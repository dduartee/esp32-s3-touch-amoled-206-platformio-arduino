#include "system_manager.hpp"

bool SystemManager::init(HWCDC &usbSerial) {
    // Store pointers to passed objects
    this->usbSerial = &usbSerial;
    
    // Initialize logger
    logger.init(&usbSerial);
    
    logger.header("SystemManager Initialization");

    // init power button
    pinMode(BTN_BOOT, INPUT_PULLUP);
    
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
    if (!display.init()) {
        logger.failure("DISPLAY", "CO5300 initialization failed");
        logger.footer();
        return false;
    }

    if(!fsManager.begin()) {
        logger.failure("LittleFS", "Failed to initialize LittleFS");
        logger.footer();
        return false;
    }

    logger.success("SYSTEM", "All components initialized successfully");
    logger.footer();
    
    this->initialized = true;
    return true;
}

void SystemManager::update() {
    static unsigned long lastTime = 0;
    
    // Simple button check
    if (buttonPressed(BTN_BOOT)) {
        this->sleep();
    }
    
    // Heartbeat every 5 seconds
    if (millis() - lastTime > 5000) {
        lastTime = millis();
        this->logHeartbeat();
    }
}

void SystemManager::sleep() {
    logger.info("SYSTEM", "Entering light sleep mode...");

    if(!sleeping) {
        display.powerOff();
        delay(50); // Display sicher abschalten

        // TODO: sleep peripherals (I2C, PMU, etc.)

        // Warten bis Button losgelassen (HIGH)
        while (digitalRead(BTN_BOOT) == LOW) {
            delay(10);
        }
    }

    logger.info("SYSTEM", "Button released, preparing for light sleep...");

    sleeping = true;
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_BOOT, 0); // Wakeup on LOW
    esp_sleep_enable_timer_wakeup(5000000); // Wakeup after 5 seconds (microseconds)
    esp_light_sleep_start();

    // Nach Light Sleep: Display neu initialisieren
    logger.info("SYSTEM", "Waking up from light sleep...");
    wakeup();
}

void SystemManager::wakeup() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        logger.info("SYSTEM", "Woke up by button press");
        display.powerOn();
        sleeping = false;
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

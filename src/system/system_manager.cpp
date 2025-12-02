#include "system_manager.hpp"
#include <cstring>

SystemManager::SystemManager(Logger* logger)
    : logger(logger), pmu(logger), display(logger), touchController(logger), fsManager(logger), rtc(logger), imu(logger)
{
    logger->header("SystemManager Initialization");

    // init power button
    pinMode(BTN_BOOT, INPUT_PULLUP);
    
    // Initialize I2C bus (100kHz Standard Mode)
    logger->info("I2C", "Initializing bus at 100kHz...");
    Wire.begin(I2C_SDA, I2C_SCL, 100000);
    this->i2c = &Wire;
    
    Serial.println("I2C scanner start");
    for (uint8_t addr = 1; addr < 127; ++addr) {
        i2c->beginTransmission(addr);
        int r = i2c->endTransmission();
        if (r == 0) {
        Serial.printf("Found I2C device at 0x%02X\n", addr);
        } // ignore other errors here
    }
    Serial.println("I2C scanner done");

    logger->success("I2C", "Bus initialized at 400kHz");

    // Initialize PMU
    logger->info("PMU", "Initializing AXP2101...");
    if (!pmu.setBus(*i2c)) {
        logger->failure("PMU", "AXP2101 initialization failed");
        logger->footer();
        return;
    }

    // Initialize Display
    logger->info("DISPLAY", "Initializing CO5300 AMOLED...");
    if (!display.isInitialized()) {
        logger->failure("DISPLAY", "CO5300 initialization failed");
        logger->footer();
        return;
    }

    // Initialize Touch
    logger->info("TOUCH", "Initializing Touch Controller...");
    if (!touchController.setBus(*i2c)) {
        logger->failure("TOUCH", "Touch Controller initialization failed");
        logger->footer();
        return;
    }

    // Initialize RTC
    logger->info("RTC", "Initializing PCF85063...");
    if (!rtc.setBus(*i2c)) {
        logger->failure("RTC", "PCF85063 initialization failed");
        logger->footer();
        return;
    }
    
    // Set initial time for testing (2025-12-01 14:30:00)
    RTC::DateTime testTime;
    testTime.year = 2025;
    testTime.month = 12;
    testTime.day = 1;
    testTime.hour = 14;
    testTime.minute = 30;
    testTime.second = 0;
    testTime.weekday = 0; // Sunday
    if (rtc.setDateTime(testTime)) {
        logger->info("RTC", "Test time set: 2025-12-01 14:30:00");
    }

    // Initialize IMU
    logger->info("IMU", "Initializing QMI8658...");
    if (!imu.setBus(*i2c)) {
        logger->failure("IMU", "QMI8658 initialization failed");
        logger->footer();
        return;
    }

    // Initialize File System
    logger->info("LittleFS", "Initializing LittleFS...");
    if (!fsManager.isInitialized()) {
        logger->failure("LittleFS", "Failed to initialize LittleFS");
        logger->footer();
        return;
    }
    
    if (!initWiFi()) {
        logger->warn("WIFI", "WiFi connection unavailable - clock will fall back to cached time");
    }

    logger->success("SYSTEM", "All components initialized successfully");
    logger->footer();
    
    this->initialized = true;
    return;
}

void SystemManager::update() {
    static unsigned long lastTime = 0;
    unsigned long current_time = millis();
    unsigned long idle_time = current_time - last_activity_time;
    
    // Simple button check
    if (buttonPressed(BTN_BOOT)) {
        this->sleep();
        return;
    }

    maintainWiFi();
    updateClockDisplay();

    touchController.handleInterrupt();
    
    // Check IMU for wrist gestures (has its own rate limiting)
    // Check for wrist tilt UP to wake display
    if (imu.checkWristTilt()) {
        if (sleeping) {
            logger->info("IMU", "⌚ Wrist raise - waking display!");
            display.powerOn();
            sleeping = false;
        }

        last_activity_time = millis();
    }
    
    // Check for wrist tilt DOWN to sleep
    if (imu.checkWristTiltDown()) {
        if (!sleeping) {
            logger->info("IMU", "⌚ Wrist lowered - entering sleep");
            sleep();
        }
    }
    
    // Check RTC alarm
    if (rtc.isAlarmTriggered()) {
        logger->info("RTC", "⏰ ALARM TRIGGERED!");
        rtc.clearAlarmFlag();
        rtc.clearAlarm();
    }
    
    // Auto Sleep Logic
    if (idle_time > LIGHT_SLEEP_TIMEOUT && !sleeping) {
        logger->info("SYSTEM", "Entering light sleep (inactive >30s)");
        sleep();
    }
    
    // Heartbeat every 5 seconds
    if (millis() - lastTime > 5000) {
        lastTime = millis();
        this->logger->debug("SYSTEM", "Heartbeat log");
        this->logHeartbeat();
    }
}

bool SystemManager::initWiFi() {
    if (WIFI_CREDENTIAL_COUNT == 0) {
        if (logger) logger->warn("WIFI", "No credentials configured in wifi_credentials.h");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    for (size_t i = 0; i < WIFI_CREDENTIAL_COUNT; ++i) {
        wifiMulti.addAP(WIFI_CREDENTIALS[i].ssid, WIFI_CREDENTIALS[i].password);
    }

    if (logger) logger->info("WIFI", "Connecting to WiFi...");
    wifiConnected = (wifiMulti.run(5000) == WL_CONNECTED);

    if (wifiConnected) {
        if (logger) {
            logger->success("WIFI", (String("Connected to ") + WiFi.SSID() + String(" - IP ") + WiFi.localIP().toString()).c_str());
        }
        syncTime();
        return true;
    }

    if (logger) logger->warn("WIFI", "Failed to connect to the configured networks");
    return false;
}

void SystemManager::maintainWiFi() {
    if (WIFI_CREDENTIAL_COUNT == 0) return;

    uint8_t status = wifiMulti.run();
    bool currentlyConnected = (status == WL_CONNECTED);

    if (currentlyConnected && !wifiConnected) {
        wifiConnected = true;
        if (logger) {
            logger->success("WIFI", (String("Reconnected to ") + WiFi.SSID() + String(" - ") + WiFi.localIP().toString()).c_str());
        }
        syncTime();
    } else if (!currentlyConnected && wifiConnected) {
        wifiConnected = false;
        if (logger) logger->warn("WIFI", "WiFi connection lost");
    }

    if (wifiConnected) {
        unsigned long now = millis();
        if (now - lastTimeSyncAttempt > TIME_SYNC_INTERVAL) {
            syncTime();
        }
    }
}

bool SystemManager::syncTime() {
    if (!wifiConnected) return false;

    lastTimeSyncAttempt = millis();
    configTime(WIFI_GMT_OFFSET_SEC, WIFI_DAYLIGHT_OFFSET, WIFI_PRIMARY_NTP, WIFI_SECONDARY_NTP);

    tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
        timeAvailable = true;
        if (logger) {
            char buffer[32];
            strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
            logger->success("TIME", (String("Synchronized: ") + String(buffer)).c_str());
        }
        return true;
    }

    if (logger) logger->warn("TIME", "Failed to obtain NTP time");
    return false;
}

void SystemManager::updateClockDisplay() {
    if (!display.isInitialized() || sleeping) return;

    unsigned long now = millis();

    if (!timeAvailable) {
        // Only update waiting screen every 1 second
        if (now - lastClockDraw < CLOCK_DRAW_INTERVAL) return;
        lastClockDraw = now;
        
        display.fillScreen(0x0000);
        display.setTextColor(0xFFFF);
        display.setTextSize(2);
        display.setCursor(20, display.getHeight() / 2 - 10);
        display.print(wifiConnected ? "Sincronizando hora..." : "Conecte-se ao WiFi");
        return;
    }

    tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        if (wifiConnected && (now - lastTimeSyncAttempt > 10000)) {
            syncTime();
        }
        return;
    }

    // Only render when time changes (not every millisecond)
    char currentTime[16];
    strftime(currentTime, sizeof(currentTime), "%H:%M:%S", &timeinfo);
    
    if (strcmp(currentTime, lastDisplayedTime) != 0 || !clockInitialized) {
        strcpy(lastDisplayedTime, currentTime);
        clockInitialized = true;
        renderClockFace(timeinfo);
    }
}

void SystemManager::renderClockFace(const tm& timeinfo) {
    // Ensure the screen is completely cleared on the first render
    if (!clockInitialized) {
        display.fillScreen(0x0000);  // Preto
    }

    uint16_t screenWidth = display.getWidth();
    uint16_t screenHeight = display.getHeight();

    // Preparar strings
    char timeStr[16];
    char dateStr[24];
    char weekDayStr[16];
    
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
    strftime(weekDayStr, sizeof(weekDayStr), "%A", &timeinfo);

    // Limpar áreas específicas onde o texto será desenhado
    display.fillRect(0, 80, screenWidth, 100, 0x0000);      // Limpar área da hora
    display.fillRect(0, 190, screenWidth, 80, 0x0000);      // Clear date area
    display.fillRect(0, 10, 200, 40, 0x0000);               // Limpar área do WiFi status

    // Desenhar hora (grande, centralizada)
    display.setTextSize(4);
    display.setTextColor(0xFFFF);  // Branco
    int16_t timeWidth = static_cast<int16_t>(strlen(timeStr) * 6 * 4);
    int16_t timeX = (screenWidth > timeWidth) ? (screenWidth - timeWidth) / 2 : 10;
    display.setCursor(timeX, 120);
    display.print(timeStr);

    // Desenhar data
    display.setTextSize(2);
    display.setTextColor(0xCCCC);  // Cinza
    int16_t dateWidth = static_cast<int16_t>(strlen(dateStr) * 6 * 2);
    int16_t dateX = (screenWidth > dateWidth) ? (screenWidth - dateWidth) / 2 : 10;
    display.setCursor(dateX, 210);
    display.print(dateStr);

    // Desenhar dia da semana
    display.setTextSize(2);
    display.setTextColor(0xCCCC);
    int16_t weekWidth = static_cast<int16_t>(strlen(weekDayStr) * 6 * 2);
    int16_t weekX = (screenWidth > weekWidth) ? (screenWidth - weekWidth) / 2 : 10;
    display.setCursor(weekX, 250);
    display.print(weekDayStr);

    // Desenhar status WiFi (canto superior esquerdo)
    display.setTextSize(1.5f);
    display.setTextColor(wifiConnected ? 0x07E0 : 0xF800);  // Verde OK, Vermelho se offline
    display.setTextSize(1);
    display.print(wifiConnected ? "WiFi OK" : "Offline");
}

void SystemManager::sleep() {
    logger->info("SYSTEM", "Entering light sleep mode...");

    if(!sleeping) {
        display.powerOff();
        delay(50); // Safely turn off display

        // TODO: sleep peripherals (I2C, PMU, etc.)

        // Wait until button is released (HIGH)
        while (digitalRead(BTN_BOOT) == LOW) {
            delay(10);
        }
    }

    logger->info("SYSTEM", "Button released, preparing for light sleep...");

    sleeping = true;
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_BOOT, 0); // Wakeup on LOW
    esp_sleep_enable_timer_wakeup(1000000); // Wakeup after 1 second (microseconds)
    esp_light_sleep_start();

    // After light sleep: reinitialize display
    logger->info("SYSTEM", "Waking up from light sleep...");
    wakeup();
}

void SystemManager::wakeup() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        logger->info("SYSTEM", "Woke up by button press");
        display.powerOn();
        sleeping = false;
        last_activity_time = millis();  // Reset idle timer!
    }
}

void SystemManager::logHeartbeat() {
    static unsigned long lastTime = 0;
    static int heartbeat = 0;
    
    // Heartbeat every 5 seconds
    if (millis() - lastTime > 5000) {
        lastTime = millis();
        heartbeat++;

        logger->header((String("SYSTEM HEARTBEAT #") + String(heartbeat)).c_str());
        logger->info("UPTIME", (String(millis() / 1000) + String(" seconds")).c_str());
        
        // Memory Status
        logger->info("MEMORY", (String("Internal RAM Free: ") + String(ESP.getFreeHeap() / 1024) + String(" KB")).c_str());
        logger->info("MEMORY", (String("PSRAM Free: ") + String(ESP.getFreePsram() / 1024) + String(" KB")).c_str());
        logger->info("MEMORY", (String("FLASH Size: ") + String(ESP.getFlashChipSize() / 1024) + String(" KB")).c_str());

        logger->info("BATTERY", (String("Battery Voltage: ") + String(this->getPMU().getBattVoltage()) + String(" mV")).c_str());
        logger->info("BATTERY", (String("Battery Percentage: ") + String(this->getPMU().getBatteryPercent()) + String(" %")).c_str());

        logger->info("BATTERY", (String("USB Connected: ") + String(this->getPMU().isUSBConnected() ? "Yes" : "No")).c_str());
        logger->info("BATTERY", (String("Battery Connected: ") + String(this->getPMU().isBatteryConnect() ? "Yes" : "No")).c_str());
        logger->info("BATTERY", (String("Charging: ") + String(this->getPMU().isCharging() ? "Yes" : "No")).c_str());

        // RTC Status
        if (rtc.isInitialized()) {
            RTC::DateTime dt;
            if (rtc.getDateTime(dt)) {
                char timeStr[32];
                snprintf(timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d", 
                         dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
                logger->info("RTC", (String("Current Time: ") + String(timeStr)).c_str());
            } else {
                logger->warn("RTC", "Failed to read time");
            }
        }
        
        // IMU Status
        if (imu.isInitialized()) {
            IMU::AccelData accel;
            IMU::GyroData gyro;
            float temp;
            
            if (imu.readAccel(accel)) {
                logger->info("IMU", (String("Accel: X=") + String(accel.x, 2) + "g Y=" + String(accel.y, 2) + "g Z=" + String(accel.z, 2) + "g").c_str());
            }
            
            if (imu.readGyro(gyro)) {
                logger->info("IMU", (String("Gyro: X=") + String(gyro.x, 1) + "°/s Y=" + String(gyro.y, 1) + "°/s Z=" + String(gyro.z, 1) + "°/s").c_str());
            }
            
            if (imu.readTemperature(temp)) {
                logger->info("IMU", (String("Temperature: ") + String(temp, 1) + "°C").c_str());
            }
        }
        
        logger->footer();
    }
}

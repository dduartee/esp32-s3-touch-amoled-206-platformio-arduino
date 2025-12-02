#pragma once

#include <Arduino.h>
#include <HWCDC.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <time.h>

#include "button/button.hpp"
#include "config.h"
#include "display/display.hpp"
#include "imu/imu.hpp"
#include "pmu/pmu.hpp"
#include "rtc/rtc.hpp"
#include "storage/fs_manager.hpp"
#include "touch/touch_controller.hpp"
#include "wifi_credentials.h"

class SystemManager {
 private:
  bool initialized = false;
  bool sleeping = false;
  unsigned long last_activity_time = 0;
  static constexpr unsigned long LIGHT_SLEEP_TIMEOUT = 30000;   // 30 seconds
  static constexpr unsigned long CLOCK_DRAW_INTERVAL = 1000;    // 1 second
  static constexpr unsigned long TIME_SYNC_INTERVAL = 3600000;  // 1 hour

  Logger* logger = nullptr;
  TwoWire* i2c = nullptr;
  PMU pmu;
  FSManager fsManager;
  Display display;
  TouchController touchController;
  RTC rtc;
  IMU imu;
  WiFiMulti wifiMulti;
  bool wifiConnected = false;
  bool timeAvailable = false;
  unsigned long lastClockDraw = 0;
  unsigned long lastTimeSyncAttempt = 0;
  char lastDisplayedTime[16] = {0};
  bool clockInitialized = false;

  void sleep();
  void wakeup();
  void logHeartbeat();
  bool initWiFi();
  void maintainWiFi();
  bool syncTime();
  void updateClockDisplay();

 public:
  void renderClockFace(const tm& timeinfo);
  SystemManager(Logger* logger);
  bool isInitialized() const { return initialized; }
  PMU& getPMU() { return pmu; }
  Display& getDisplay() { return display; }
  RTC& getRTC() { return rtc; }
  IMU& getIMU() { return imu; }
  Logger* getLogger() { return logger; }
  TwoWire* getI2C() { return i2c; }

  void update();
};
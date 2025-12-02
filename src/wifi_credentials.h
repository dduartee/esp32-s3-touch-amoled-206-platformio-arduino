#pragma once
#include <cstddef>

/**
 * Configure your WiFi networks here. Add as many entries as needed, the
 * WiFiMulti helper will try them sequentially until a connection succeeds.
 */
struct WiFiCredential {
    const char* ssid;
    const char* password;
};

static constexpr WiFiCredential WIFI_CREDENTIALS[] = {
    {"SPIKE_2", "casa526610"},
    {"OfficeWiFi", "fallback_password"}
};

static constexpr size_t WIFI_CREDENTIAL_COUNT = sizeof(WIFI_CREDENTIALS) / sizeof(WIFI_CREDENTIALS[0]);

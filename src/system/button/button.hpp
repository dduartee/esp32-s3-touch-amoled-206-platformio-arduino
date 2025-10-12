#pragma once
#include <Arduino.h>

// Einfache Button-Funktion mit Entprellung
inline bool buttonPressed(uint8_t pin) {
    static unsigned long lastPress[40] = {0};
    static bool lastState[40] = {HIGH};
    bool currentState = digitalRead(pin);
    if (currentState == LOW && lastState[pin] == HIGH && (millis() - lastPress[pin]) > 100) {
        lastPress[pin] = millis();
        lastState[pin] = currentState;
        return true;
    }
    lastState[pin] = currentState;
    return false;
}
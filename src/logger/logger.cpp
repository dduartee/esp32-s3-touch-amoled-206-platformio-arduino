#include "logger.hpp"

void Logger::print(const char* level, const char* component, const char* message) {
    if (!initialized) return;
    serial->print("[");
    serial->print(level);
    serial->print("] ");
    serial->print(component);
    serial->print(": ");
    serial->println(message);
}

void Logger::error(const char* component, const char* message) {
    if (!initialized) return;
    print("ERROR", component, message);
}

void Logger::warn(const char* component, const char* message) {
    if (!initialized) return;
    print("WARN", component, message);
}

void Logger::info(const char* component, const char* message) {
    if (!initialized) return;
    print("INFO", component, message);
}

void Logger::debug(const char* component, const char* message) {
    if (!initialized) return;
    print("DEBUG", component, message);
}

void Logger::success(const char* component, const char* message) {
    if (!initialized) return;
    print("SUCCESS", component, message);
}

void Logger::failure(const char* component, const char* message) {
    if (!initialized) return;
    print("FAILURE", component, message);
}

void Logger::header(const char* title) {
    if (!initialized) return;
    serial->println("==========================================");
    serial->print("| ");
    serial->println(title);
    serial->println("==========================================");
}

void Logger::footer() {
    if (!initialized) return;
    serial->println("==========================================");
}

void Logger::println(const char* message) {
    if (!initialized) return;
    serial->println(message);
}

#include "fs_manager.hpp"

FSManager::FSManager(Logger* logger) {
    this->logger = logger;

    // Try to mount LittleFS first without formatting
    if(!LittleFS.begin(false)) {
        logger->info("FSManager", "Initial mount failed, attempting format...");
        if(!LittleFS.begin(true)) {
            logger->failure("FSManager", "Failed to mount LittleFS even after formatting");
            logger->footer();
            return;
        }
        logger->success("FSManager", "LittleFS formatted and mounted successfully");
    } else {
        logger->success("FSManager", "LittleFS mounted successfully");
    }

    logger->info("FSManager", String("Total space: " + String(totalKB()) + " KB").c_str());
    logger->info("FSManager", String("Used space: " + String(usedKB()) + " KB").c_str());
    logger->info("FSManager", String("Free space: " + String(totalKB() - usedKB()) + " KB").c_str());

    initialized = true;
}

FSManager::~FSManager() {
    LittleFS.end();
}

bool FSManager::writeFile(const char* path, const String& data) {
    logger->debug("FSManager", String("Writing to file: " + String(path)).c_str());
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        logger->failure("FSManager", String("Failed to open file for writing: " + String(path)).c_str());
        return false;
    }
    
    file.print(data);
    file.close();

    logger->success("FSManager", String("Successfully wrote to file: " + String(path)).c_str());
    
    return true;
}

String FSManager::readFile(const char* path) {
    logger->debug("FSManager", String("Reading file: " + String(path)).c_str());
    
    File file = LittleFS.open(path);
    
    if (!file || file.isDirectory()) {
        logger->failure("FSManager", String("Failed to open file for reading or it's a directory: " + String(path)).c_str());
        return "";
    }
    
    String out;
    
    while (file.available()) {
        out += file.readStringUntil('\n') + "\n";
    }

    file.close();
    
    logger->success("FSManager", String("Successfully read file: " + String(path)).c_str());

    return out;
}
#include "esp32_flash_storage.hpp"
#include "../../logger.hpp"

bool ESP32FlashStorage::init() {
    logger.info("ESP32_FLASH", "Initializing 32MB Flash Storage (ESP32 API)...");
    
    // Get flash chip information
    uint32_t flashSize = ESP.getFlashChipSize();
    uint32_t flashSpeed = ESP.getFlashChipSpeed();
    
    logger.info("ESP32_FLASH", String("Detected flash size: " + String(flashSize / 1024 / 1024) + " MB").c_str());
    logger.info("ESP32_FLASH", String("Flash speed: " + String(flashSpeed / 1000000) + " MHz").c_str());
    
    // Verify we have the expected 32MB flash
    if (flashSize < TOTAL_FLASH_SIZE) {
        logger.failure("ESP32_FLASH", String("Insufficient flash size - need 32MB, have " + 
                      String(flashSize / 1024 / 1024) + "MB").c_str());
        return false;
    }
    
    // Check ESP32 flash chip handle
    esp_flash_t* flashChip = esp_flash_default_chip;
    if (!flashChip) {
        logger.failure("ESP32_FLASH", "No default flash chip found");
        return false;
    }
    
    size_t chipSize;
    esp_err_t err = esp_flash_get_size(flashChip, &chipSize);
    if (err != ESP_OK) {
        logger.failure("ESP32_FLASH", String("Failed to get chip size: " + String(esp_err_to_name(err))).c_str());
        return false;
    }
    
    // List existing partitions to see what's used
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    logger.info("ESP32_FLASH", "Existing partitions:");
    
    uint32_t maxUsedAddress = 0;
    while (it != NULL) {
        const esp_partition_t* partition = esp_partition_get(it);
        logger.info("ESP32_FLASH", String("  " + String(partition->label) + 
                   ": 0x" + String(partition->address, HEX) + 
                   " - 0x" + String(partition->address + partition->size, HEX) +
                   " (" + String(partition->size / 1024) + "KB)").c_str());
        
        uint32_t partitionEnd = partition->address + partition->size;
        if (partitionEnd > maxUsedAddress) {
            maxUsedAddress = partitionEnd;
        }
        
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    
    // Calculate actual free space
    uint32_t freeSpaceStart = (maxUsedAddress + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1); // Align to sector
    uint32_t freeSpaceSize = flashSize - freeSpaceStart;
    
    logger.info("ESP32_FLASH", String("Free space starts at: 0x" + String(freeSpaceStart, HEX)).c_str());
    logger.info("ESP32_FLASH", String("Free space size: " + String(freeSpaceSize / 1024 / 1024) + " MB").c_str());

    logger.success("ESP32_FLASH", "32MB Flash initialized successfully");
    logger.info("ESP32_FLASH", String("User data area: " + String(USER_DATA_SIZE / 1024 / 1024) + 
                "MB at offset 0x" + String(USER_DATA_START, HEX)).c_str());
    
    initialized = true;
    return true;
}

String ESP32FlashStorage::getFlashInfo() {
    String info = "Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + "MB";
    info += ", Speed: " + String(ESP.getFlashChipSpeed() / 1000000) + "MHz";
    info += ", Mode: " + String(ESP.getFlashChipMode());
    info += ", User Area: " + String(USER_DATA_SIZE / 1024 / 1024) + "MB";
    info += " @ 0x" + String(USER_DATA_START, HEX);
    
    return info;
}

bool ESP32FlashStorage::isValidOffset(uint32_t offset, uint32_t length) const {
    return (offset + length <= USER_DATA_SIZE);
}

uint32_t ESP32FlashStorage::toFlashAddress(uint32_t offset) const {
    return USER_DATA_START + offset;
}

bool ESP32FlashStorage::writeData(uint32_t offset, const uint8_t* data, uint32_t length) {
    if (!initialized) {
        logger.failure("ESP32_FLASH", "Not initialized");
        return false;
    }
    
    if (!isValidOffset(offset, length)) {
        logger.failure("ESP32_FLASH", String("Write offset out of bounds: " + String(offset) + 
                      " + " + String(length) + " > " + String(USER_DATA_SIZE)).c_str());
        return false;
    }
    
    uint32_t flashAddr = toFlashAddress(offset);
    
    // Erase sectors if needed
    uint32_t startSector = (flashAddr / SECTOR_SIZE) * SECTOR_SIZE;
    uint32_t endAddr = flashAddr + length;
    uint32_t endSector = ((endAddr + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    
    // Erase required sectors
    for (uint32_t addr = startSector; addr < endSector; addr += SECTOR_SIZE) {
        esp_err_t err = esp_flash_erase_region(esp_flash_default_chip, addr, SECTOR_SIZE);
        if (err != ESP_OK) {
            logger.failure("ESP32_FLASH", String("Sector erase failed at 0x" + String(addr, HEX) + 
                          ": " + String(esp_err_to_name(err))).c_str());
            return false;
        }
    }
    
    // Write data
    esp_err_t err = esp_flash_write(esp_flash_default_chip, data, flashAddr, length);
    if (err != ESP_OK) {
        logger.failure("ESP32_FLASH", String("Write failed at 0x" + String(flashAddr, HEX) + 
                      ": " + String(esp_err_to_name(err))).c_str());
        return false;
    }
    
    logger.debug("ESP32_FLASH", String("Written " + String(length) + " bytes at offset 0x" + String(offset, HEX)).c_str());
    return true;
}

bool ESP32FlashStorage::readData(uint32_t offset, uint8_t* data, uint32_t length) {
    if (!initialized) {
        logger.failure("ESP32_FLASH", "Not initialized");
        return false;
    }
    
    if (!isValidOffset(offset, length)) {
        logger.failure("ESP32_FLASH", "Read offset out of bounds");
        return false;
    }
    
    uint32_t flashAddr = toFlashAddress(offset);
    
    esp_err_t err = esp_flash_read(esp_flash_default_chip, data, flashAddr, length);
    if (err != ESP_OK) {
        logger.failure("ESP32_FLASH", String("Read failed at 0x" + String(flashAddr, HEX) + 
                      ": " + String(esp_err_to_name(err))).c_str());
        return false;
    }
    
    logger.debug("ESP32_FLASH", String("Read " + String(length) + " bytes from offset 0x" + String(offset, HEX)).c_str());
    return true;
}

bool ESP32FlashStorage::eraseSector(uint32_t offset) {
    if (!initialized) {
        logger.failure("ESP32_FLASH", "Not initialized");
        return false;
    }
    
    if (!isValidOffset(offset)) {
        logger.failure("ESP32_FLASH", "Erase offset out of bounds");
        return false;
    }
    
    uint32_t flashAddr = toFlashAddress(offset);
    uint32_t sectorAddr = (flashAddr / SECTOR_SIZE) * SECTOR_SIZE;
    
    esp_err_t err = esp_flash_erase_region(esp_flash_default_chip, sectorAddr, SECTOR_SIZE);
    if (err != ESP_OK) {
        logger.failure("ESP32_FLASH", String("Sector erase failed at 0x" + String(sectorAddr, HEX) + 
                      ": " + String(esp_err_to_name(err))).c_str());
        return false;
    }
    
    logger.debug("ESP32_FLASH", String("Erased sector at 0x" + String(sectorAddr, HEX)).c_str());
    return true;
}

bool ESP32FlashStorage::eraseRange(uint32_t offset, uint32_t length) {
    if (!initialized) {
        logger.failure("ESP32_FLASH", "Not initialized");
        return false;
    }
    
    if (!isValidOffset(offset, length)) {
        logger.failure("ESP32_FLASH", "Erase range out of bounds");
        return false;
    }
    
    // Ensure sector alignment
    if (offset % SECTOR_SIZE != 0 || length % SECTOR_SIZE != 0) {
        logger.failure("ESP32_FLASH", "Erase range must be sector aligned (4KB)");
        return false;
    }
    
    uint32_t flashAddr = toFlashAddress(offset);
    
    esp_err_t err = esp_flash_erase_region(esp_flash_default_chip, flashAddr, length);
    if (err != ESP_OK) {
        logger.failure("ESP32_FLASH", String("Range erase failed at 0x" + String(flashAddr, HEX) + 
                      ": " + String(esp_err_to_name(err))).c_str());
        return false;
    }
    
    logger.debug("ESP32_FLASH", String("Erased " + String(length) + " bytes at offset 0x" + String(offset, HEX)).c_str());
    return true;
}

bool ESP32FlashStorage::writeString(uint32_t offset, const String& text) {
    uint32_t length = text.length();
    
    // Check bounds for header + string
    if (!isValidOffset(offset, sizeof(length) + length)) {
        return false;
    }
    
    // Write length header (4 bytes)
    if (!writeData(offset, (uint8_t*)&length, sizeof(length))) {
        return false;
    }
    
    // Write string data
    return writeData(offset + sizeof(length), (uint8_t*)text.c_str(), length);
}

String ESP32FlashStorage::readString(uint32_t offset, uint32_t maxLength) {
    // Read length header
    uint32_t length;
    if (!readData(offset, (uint8_t*)&length, sizeof(length))) {
        return "";
    }
    
    // Sanity check
    if (length > maxLength || length == 0 || length > 65535) {
        logger.failure("ESP32_FLASH", String("Invalid string length: " + String(length)).c_str());
        return "";
    }
    
    // Check bounds
    if (!isValidOffset(offset + sizeof(length), length)) {
        return "";
    }
    
    // Read string data
    char* buffer = new char[length + 1];
    if (!readData(offset + sizeof(length), (uint8_t*)buffer, length)) {
        delete[] buffer;
        return "";
    }
    
    buffer[length] = '\0';
    String result = String(buffer);
    delete[] buffer;
    
    return result;
}

bool ESP32FlashStorage::writeFile(uint32_t offset, const uint8_t* data, uint32_t length) {
    // Check bounds for header + data
    if (!isValidOffset(offset, sizeof(length) + length)) {
        return false;
    }
    
    // Write size header (4 bytes)
    if (!writeData(offset, (uint8_t*)&length, sizeof(length))) {
        return false;
    }
    
    // Write file data
    return writeData(offset + sizeof(length), data, length);
}

uint32_t ESP32FlashStorage::readFile(uint32_t offset, uint8_t* data, uint32_t maxLength) {
    // Read size header
    uint32_t length;
    if (!readData(offset, (uint8_t*)&length, sizeof(length))) {
        return 0;
    }
    
    // Sanity check
    if (length > maxLength || length == 0) {
        return 0;
    }
    
    // Check bounds
    if (!isValidOffset(offset + sizeof(length), length)) {
        return 0;
    }
    
    // Read file data
    if (!readData(offset + sizeof(length), data, length)) {
        return 0;
    }
    
    return length;
}

bool ESP32FlashStorage::runTest() {
    logger.info("ESP32_FLASH", "Running flash test...");
    
    if (!initialized) {
        logger.failure("ESP32_FLASH", "Flash not initialized");
        return false;
    }
    
    // Test in safe area (start of free space)
    uint32_t testOffset = 0x000000;  // Start of user data area (0x800000 absolute)

    logger.info("ESP32_FLASH", String("Testing at offset 0x" + String(testOffset, HEX)).c_str());

    // Test data
    String testString = "Hello 24MB ESP32 Flash!";
    uint8_t testData[] = {0xAA, 0x55, 0xFF, 0x00, 0x12, 0x34, 0x56, 0x78};
    
    // Erase test sector
    if (!eraseSector(testOffset)) {
        logger.failure("ESP32_FLASH", "Test sector erase failed");
        return false;
    }
    
    // Test string write/read
    if (!writeString(testOffset, testString)) {
        logger.failure("ESP32_FLASH", "String write failed");
        return false;
    }
    
    String readBack = readString(testOffset);
    if (readBack != testString) {
        logger.failure("ESP32_FLASH", String("String mismatch - wrote: '" + testString + 
                      "', read: '" + readBack + "'").c_str());
        return false;
    }
    
    // Test raw data write/read
    uint32_t dataOffset = testOffset + 1024;  // Offset from string test
    if (!writeData(dataOffset, testData, sizeof(testData))) {
        logger.failure("ESP32_FLASH", "Data write failed");
        return false;
    }
    
    uint8_t readBuffer[sizeof(testData)];
    if (!readData(dataOffset, readBuffer, sizeof(testData))) {
        logger.failure("ESP32_FLASH", "Data read failed");
        return false;
    }
    
    // Verify data
    for (size_t i = 0; i < sizeof(testData); i++) {
        if (readBuffer[i] != testData[i]) {
            logger.failure("ESP32_FLASH", String("Data mismatch at byte " + String(i) + 
                          " - expected 0x" + String(testData[i], HEX) + 
                          ", got 0x" + String(readBuffer[i], HEX)).c_str());
            return false;
        }
    }
    
    logger.success("ESP32_FLASH", "All tests passed!");
    logger.info("ESP32_FLASH", String("String test: '" + readBack + "'").c_str());
    logger.info("ESP32_FLASH", String("Data test: " + String(sizeof(testData)) + " bytes verified").c_str());

    return true;
}
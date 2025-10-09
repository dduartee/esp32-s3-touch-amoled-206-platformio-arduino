#pragma once

#include <Arduino.h>
#include <esp_flash.h>
#include <esp_partition.h>
#include <vector>

/**
 * ESP32 Flash Storage Manager
 * Direct access to unused areas of 32MB NOR Flash using ESP32 Flash API
 * Safer than SPIMemory as it respects ESP32 flash management
 */
class ESP32FlashStorage {
private:
    bool initialized = false;
    
    // Flash parameters  
    static constexpr uint32_t TOTAL_FLASH_SIZE = 32 * 1024 * 1024;  // 32MB
    static constexpr uint32_t USER_DATA_START = 0x670000;           // Use SPIFFS area (safe & tested)
    static constexpr uint32_t USER_DATA_SIZE = 1536 * 1024;         // 1536KB available (SPIFFS area)
    static constexpr uint32_t SECTOR_SIZE = 4096;                   // 4KB sectors
    
public:
    /**
     * Initialize ESP32 Flash Storage
     * @return true if successful
     */
    bool init();

    /**
     * Check if flash is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized; }

    /**
     * Get flash chip information
     * @return info string with chip details
     */
    String getFlashInfo();

    /**
     * Get total user data area size
     * @return available bytes for user data (24MB)
     */
    uint32_t getTotalUserSize() const { return USER_DATA_SIZE; }

    /**
     * Get user data start offset
     * @return start address of user data area
     */
    uint32_t getUserDataStart() const { return USER_DATA_START; }

    /**
     * Write data to user flash area
     * @param offset offset within user data area (0 to USER_DATA_SIZE)
     * @param data pointer to data buffer
     * @param length number of bytes to write
     * @return true if successful
     */
    bool writeData(uint32_t offset, const uint8_t* data, uint32_t length);

    /**
     * Read data from user flash area
     * @param offset offset within user data area
     * @param data pointer to data buffer
     * @param length number of bytes to read
     * @return true if successful
     */
    bool readData(uint32_t offset, uint8_t* data, uint32_t length);

    /**
     * Erase sector in user flash area
     * @param offset any offset within the sector to erase
     * @return true if successful
     */
    bool eraseSector(uint32_t offset);

    /**
     * Erase range of user flash area
     * @param offset start offset (must be sector aligned)
     * @param length length to erase (must be multiple of sector size)
     * @return true if successful
     */
    bool eraseRange(uint32_t offset, uint32_t length);

    /**
     * Write string to user flash area with length header
     * @param offset offset within user data area
     * @param text string to write
     * @return true if successful
     */
    bool writeString(uint32_t offset, const String& text);

    /**
     * Read string from user flash area
     * @param offset offset within user data area
     * @param maxLength maximum string length
     * @return string content, empty if failed
     */
    String readString(uint32_t offset, uint32_t maxLength = 4096);

    /**
     * Simple file interface: write file with size header
     * @param offset offset within user data area
     * @param data file data
     * @param length file size
     * @return true if successful
     */
    bool writeFile(uint32_t offset, const uint8_t* data, uint32_t length);

    /**
     * Simple file interface: read file with size header
     * @param offset offset within user data area
     * @param data buffer to read into
     * @param maxLength maximum buffer size
     * @return actual bytes read, 0 if failed
     */
    uint32_t readFile(uint32_t offset, uint8_t* data, uint32_t maxLength);

    /**
     * Test flash functionality in safe area
     * @return true if all tests passed
     */
    bool runTest();

    /**
     * Check if offset is within user data bounds
     * @param offset offset to check
     * @param length data length
     * @return true if valid
     */
    bool isValidOffset(uint32_t offset, uint32_t length = 1) const;

private:
    /**
     * Convert user offset to absolute flash address
     * @param offset user data offset
     * @return absolute flash address
     */
    uint32_t toFlashAddress(uint32_t offset) const;
};
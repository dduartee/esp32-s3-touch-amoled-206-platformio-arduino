#pragma once

#include <Arduino.h>
#include <vector>
#include <map>
#include "esp32_flash_storage.hpp"

/**
 * Simple File System on top of ESP32FlashStorage
 * Manages file allocation, sizes, and provides file-like interface
 */
class FlashFileSystem {
public:
    struct FileInfo {
        String name;
        uint32_t address;
        uint32_t size;
        uint32_t timestamp;
    };

private:
    ESP32FlashStorage* flashStorage;
    bool initialized = false;
    
    // File system layout (using 1536KB SPIFFS area)
    static constexpr uint32_t FILE_TABLE_START = 0x000000;     // 0-64KB: File allocation table
    static constexpr uint32_t FILE_TABLE_SIZE = 0x010000;      // 64KB for file table
    static constexpr uint32_t DATA_AREA_START = 0x010000;      // 64KB+: Actual file data
    static constexpr uint32_t DATA_AREA_SIZE = 0x170000;       // ~1472KB for files (1536-64)
    static constexpr uint32_t MAX_FILES = 512;                 // Max files in system
    static constexpr uint32_t ALLOCATION_BLOCK = 4096;         // 4KB allocation blocks
    
    std::map<String, FileInfo> fileTable;
    std::vector<bool> allocationBitmap;  // Track allocated blocks
    
public:
    /**
     * Initialize file system
     * @param storage ESP32FlashStorage instance
     * @return true if successful
     */
    bool init(ESP32FlashStorage* storage);

    /**
     * Check if file system is ready
     */
    bool isInitialized() const { return initialized; }

    /**
     * Write file (automatically finds space)
     * @param filename file name (max 31 chars)
     * @param data file data
     * @param size file size
     * @return true if successful
     */
    bool writeFile(const String& filename, const uint8_t* data, uint32_t size);

    /**
     * Read entire file
     * @param filename file name
     * @param buffer output buffer
     * @param maxSize maximum buffer size
     * @return actual bytes read, 0 if failed
     */
    uint32_t readFile(const String& filename, uint8_t* buffer, uint32_t maxSize);

    /**
     * Read file as string
     * @param filename file name
     * @return file content as string, empty if failed
     */
    String readFileAsString(const String& filename);

    /**
     * Write string as file
     * @param filename file name
     * @param content string content
     * @return true if successful
     */
    bool writeStringAsFile(const String& filename, const String& content);

    /**
     * Delete file
     * @param filename file name
     * @return true if successful
     */
    bool deleteFile(const String& filename);

    /**
     * Check if file exists
     * @param filename file name
     * @return true if file exists
     */
    bool exists(const String& filename);

    /**
     * Get file size
     * @param filename file name
     * @return file size in bytes, 0 if not found
     */
    uint32_t getFileSize(const String& filename);

    /**
     * List all files
     * @return vector of file names
     */
    std::vector<String> listFiles();

    /**
     * Get file information
     * @param filename file name
     * @return FileInfo struct, empty if not found
     */
    FileInfo getFileInfo(const String& filename);

    /**
     * Get file system statistics
     */
    struct FSStats {
        uint32_t totalSpace;
        uint32_t usedSpace;
        uint32_t freeSpace;
        uint32_t fileCount;
        uint32_t largestFreeBlock;
    };
    FSStats getStats();

    /**
     * Format file system (delete all files)
     * @return true if successful
     */
    bool format();

private:
    /**
     * Load file table from flash
     */
    bool loadFileTable();

    /**
     * Save file table to flash
     */
    bool saveFileTable();

    /**
     * Find free space for file of given size
     * @param size required size in bytes
     * @return start address, 0 if no space
     */
    uint32_t findFreeSpace(uint32_t size);

    /**
     * Mark blocks as allocated/free
     * @param startAddr start address
     * @param size size in bytes
     * @param allocated true to mark as allocated, false for free
     */
    void updateAllocation(uint32_t startAddr, uint32_t size, bool allocated);

    /**
     * Convert address to block index
     */
    uint32_t addressToBlock(uint32_t address);

    /**
     * Convert block index to address
     */
    uint32_t blockToAddress(uint32_t block);

    /**
     * Calculate required blocks for size
     */
    uint32_t sizeToBlocks(uint32_t size);
};
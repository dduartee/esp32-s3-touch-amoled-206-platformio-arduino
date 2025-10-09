#include "flash_file_system.hpp"
#include "../../logger.hpp"

bool FlashFileSystem::init(ESP32FlashStorage* storage) {
    logger.info("FLASH_FS", "Initializing Flash File System...");
    
    if (!storage || !storage->isInitialized()) {
        logger.failure("FLASH_FS", "Invalid or uninitialized storage");
        return false;
    }
    
    flashStorage = storage;
    
    // Initialize allocation bitmap
    uint32_t totalBlocks = DATA_AREA_SIZE / ALLOCATION_BLOCK;
    allocationBitmap.resize(totalBlocks, false);
    
    logger.info("FLASH_FS", ("Data area: " + String(DATA_AREA_SIZE / 1024 / 1024) + "MB").c_str());
    logger.info("FLASH_FS", ("Block size: " + String(ALLOCATION_BLOCK) + " bytes").c_str());
    logger.info("FLASH_FS", ("Total blocks: " + String(totalBlocks)).c_str());
    
    // Try to load existing file table
    if (!loadFileTable()) {
        logger.info("FLASH_FS", "No existing file table found, creating new one");
        fileTable.clear();
    }
    
    logger.success("FLASH_FS", "File system initialized");
    logger.info("FLASH_FS", ("Files found: " + String(fileTable.size())).c_str());
    
    initialized = true;
    return true;
}

bool FlashFileSystem::writeFile(const String& filename, const uint8_t* data, uint32_t size) {
    if (!initialized) {
        logger.failure("FLASH_FS", "File system not initialized");
        return false;
    }
    
    if (filename.length() > 31) {
        logger.failure("FLASH_FS", ("Filename too long: " + filename).c_str());
        return false;
    }
    
    if (size == 0) {
        logger.failure("FLASH_FS", "Cannot write empty file");
        return false;
    }
    
    // Delete existing file if it exists
    if (exists(filename)) {
        deleteFile(filename);
    }
    
    // Find free space
    uint32_t address = findFreeSpace(size);
    if (address == 0) {
        logger.failure("FLASH_FS", ("No space for file: " + filename + " (" + String(size) + " bytes)").c_str());
        return false;
    }
    
    // Write file data
    if (!flashStorage->writeData(address, data, size)) {
        logger.failure("FLASH_FS", ("Failed to write file data: " + filename).c_str());
        return false;
    }
    
    // Add to file table
    FileInfo info;
    info.name = filename;
    info.address = address;
    info.size = size;
    info.timestamp = millis();
    
    fileTable[filename] = info;
    updateAllocation(address, size, true);
    
    // Save file table
    if (!saveFileTable()) {
        logger.failure("FLASH_FS", "Failed to save file table");
        return false;
    }
    
    logger.success("FLASH_FS", ("File written: " + filename + " (" + String(size) + " bytes)").c_str());
    return true;
}

uint32_t FlashFileSystem::readFile(const String& filename, uint8_t* buffer, uint32_t maxSize) {
    if (!initialized) {
        logger.failure("FLASH_FS", "File system not initialized");
        return 0;
    }
    
    auto it = fileTable.find(filename);
    if (it == fileTable.end()) {
        logger.failure("FLASH_FS", ("File not found: " + filename).c_str());
        return 0;
    }
    
    FileInfo& info = it->second;
    uint32_t readSize = (info.size > maxSize) ? maxSize : info.size;
    
    if (!flashStorage->readData(info.address, buffer, readSize)) {
        logger.failure("FLASH_FS", ("Failed to read file: " + filename).c_str());
        return 0;
    }
    
    logger.debug("FLASH_FS", ("File read: " + filename + " (" + String(readSize) + " bytes)").c_str());
    return readSize;
}

String FlashFileSystem::readFileAsString(const String& filename) {
    if (!exists(filename)) {
        return "";
    }
    
    FileInfo info = getFileInfo(filename);
    if (info.size == 0 || info.size > 65535) {  // Sanity check
        return "";
    }
    
    char* buffer = new char[info.size + 1];
    uint32_t bytesRead = readFile(filename, (uint8_t*)buffer, info.size);
    
    if (bytesRead != info.size) {
        delete[] buffer;
        return "";
    }
    
    buffer[bytesRead] = '\0';
    String result = String(buffer);
    delete[] buffer;
    
    return result;
}

bool FlashFileSystem::writeStringAsFile(const String& filename, const String& content) {
    return writeFile(filename, (const uint8_t*)content.c_str(), content.length());
}

bool FlashFileSystem::deleteFile(const String& filename) {
    if (!initialized) {
        logger.failure("FLASH_FS", "File system not initialized");
        return false;
    }
    
    auto it = fileTable.find(filename);
    if (it == fileTable.end()) {
        logger.failure("FLASH_FS", ("File not found: " + filename).c_str());
        return false;
    }
    
    FileInfo& info = it->second;
    
    // Mark space as free
    updateAllocation(info.address, info.size, false);
    
    // Remove from file table
    fileTable.erase(it);
    
    // Save file table
    if (!saveFileTable()) {
        logger.failure("FLASH_FS", "Failed to save file table after delete");
        return false;
    }
    
    logger.success("FLASH_FS", ("File deleted: " + filename).c_str());
    return true;
}

bool FlashFileSystem::exists(const String& filename) {
    return fileTable.find(filename) != fileTable.end();
}

uint32_t FlashFileSystem::getFileSize(const String& filename) {
    auto it = fileTable.find(filename);
    return (it != fileTable.end()) ? it->second.size : 0;
}

std::vector<String> FlashFileSystem::listFiles() {
    std::vector<String> files;
    for (const auto& pair : fileTable) {
        files.push_back(pair.first);
    }
    return files;
}

FlashFileSystem::FileInfo FlashFileSystem::getFileInfo(const String& filename) {
    auto it = fileTable.find(filename);
    return (it != fileTable.end()) ? it->second : FileInfo();
}

FlashFileSystem::FSStats FlashFileSystem::getStats() {
    FSStats stats;
    stats.totalSpace = DATA_AREA_SIZE;
    stats.usedSpace = 0;
    stats.fileCount = fileTable.size();
    
    // Calculate used space
    for (const auto& pair : fileTable) {
        uint32_t blocks = sizeToBlocks(pair.second.size);
        stats.usedSpace += blocks * ALLOCATION_BLOCK;
    }
    
    stats.freeSpace = stats.totalSpace - stats.usedSpace;
    
    // Find largest free block
    stats.largestFreeBlock = 0;
    uint32_t currentFreeBlock = 0;
    
    for (size_t i = 0; i < allocationBitmap.size(); i++) {
        if (!allocationBitmap[i]) {
            currentFreeBlock += ALLOCATION_BLOCK;
        } else {
            if (currentFreeBlock > stats.largestFreeBlock) {
                stats.largestFreeBlock = currentFreeBlock;
            }
            currentFreeBlock = 0;
        }
    }
    
    if (currentFreeBlock > stats.largestFreeBlock) {
        stats.largestFreeBlock = currentFreeBlock;
    }
    
    return stats;
}

bool FlashFileSystem::format() {
    logger.info("FLASH_FS", "Formatting file system...");
    
    // Clear file table
    fileTable.clear();
    
    // Clear allocation bitmap
    std::fill(allocationBitmap.begin(), allocationBitmap.end(), false);
    
    // Erase file table area
    for (uint32_t addr = FILE_TABLE_START; addr < FILE_TABLE_START + FILE_TABLE_SIZE; addr += 4096) {
        if (!flashStorage->eraseSector(addr)) {
            logger.failure("FLASH_FS", "Failed to erase file table");
            return false;
        }
    }
    
    // Save empty file table
    if (!saveFileTable()) {
        logger.failure("FLASH_FS", "Failed to save empty file table");
        return false;
    }
    
    logger.success("FLASH_FS", "File system formatted");
    return true;
}

// Private methods

bool FlashFileSystem::loadFileTable() {
    // Try to read file table from flash
    uint32_t fileCount;
    if (!flashStorage->readData(FILE_TABLE_START, (uint8_t*)&fileCount, sizeof(fileCount))) {
        return false;
    }
    
    if (fileCount == 0xFFFFFFFF || fileCount > MAX_FILES) {
        // Uninitialized flash or corrupted data
        return false;
    }
    
    logger.info("FLASH_FS", ("Loading " + String(fileCount) + " files from table").c_str());
    
    uint32_t offset = sizeof(fileCount);
    fileTable.clear();
    std::fill(allocationBitmap.begin(), allocationBitmap.end(), false);
    
    for (uint32_t i = 0; i < fileCount; i++) {
        FileInfo info;
        
        // Read file info structure
        if (!flashStorage->readData(FILE_TABLE_START + offset, (uint8_t*)&info, sizeof(FileInfo))) {
            logger.failure("FLASH_FS", ("Failed to read file info " + String(i)).c_str());
            return false;
        }
        
        fileTable[info.name] = info;
        updateAllocation(info.address, info.size, true);
        
        offset += sizeof(FileInfo);
        
        if (offset >= FILE_TABLE_SIZE) {
            logger.failure("FLASH_FS", "File table overflow");
            break;
        }
    }
    
    logger.success("FLASH_FS", ("Loaded " + String(fileTable.size()) + " files").c_str());
    return true;
}

bool FlashFileSystem::saveFileTable() {
    // Erase file table area
    if (!flashStorage->eraseSector(FILE_TABLE_START)) {
        return false;
    }
    
    uint32_t fileCount = fileTable.size();
    
    // Write file count
    if (!flashStorage->writeData(FILE_TABLE_START, (uint8_t*)&fileCount, sizeof(fileCount))) {
        return false;
    }
    
    // Write file entries
    uint32_t offset = sizeof(fileCount);
    
    for (const auto& pair : fileTable) {
        const FileInfo& info = pair.second;
        
        if (!flashStorage->writeData(FILE_TABLE_START + offset, (uint8_t*)&info, sizeof(FileInfo))) {
            logger.failure("FLASH_FS", ("Failed to write file info: " + info.name).c_str());
            return false;
        }
        
        offset += sizeof(FileInfo);
        
        if (offset >= FILE_TABLE_SIZE) {
            logger.failure("FLASH_FS", "File table full");
            break;
        }
    }
    
    return true;
}

uint32_t FlashFileSystem::findFreeSpace(uint32_t size) {
    uint32_t requiredBlocks = sizeToBlocks(size);
    uint32_t consecutiveBlocks = 0;
    uint32_t startBlock = 0;
    
    for (size_t i = 0; i < allocationBitmap.size(); i++) {
        if (!allocationBitmap[i]) {  // Free block
            if (consecutiveBlocks == 0) {
                startBlock = i;
            }
            consecutiveBlocks++;
            
            if (consecutiveBlocks >= requiredBlocks) {
                return blockToAddress(startBlock);
            }
        } else {  // Allocated block
            consecutiveBlocks = 0;
        }
    }
    
    return 0;  // No space found
}

void FlashFileSystem::updateAllocation(uint32_t startAddr, uint32_t size, bool allocated) {
    uint32_t startBlock = addressToBlock(startAddr);
    uint32_t blocks = sizeToBlocks(size);
    
    for (uint32_t i = 0; i < blocks; i++) {
        if (startBlock + i < allocationBitmap.size()) {
            allocationBitmap[startBlock + i] = allocated;
        }
    }
}

uint32_t FlashFileSystem::addressToBlock(uint32_t address) {
    return (address - DATA_AREA_START) / ALLOCATION_BLOCK;
}

uint32_t FlashFileSystem::blockToAddress(uint32_t block) {
    return DATA_AREA_START + (block * ALLOCATION_BLOCK);
}

uint32_t FlashFileSystem::sizeToBlocks(uint32_t size) {
    return (size + ALLOCATION_BLOCK - 1) / ALLOCATION_BLOCK;
}
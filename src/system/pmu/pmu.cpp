#include "pmu.hpp"

bool PMU::setBus(TwoWire &wire) {
    logger->debug("PMU", "Starting AXP2101 initialization...");
    if (!pmu.begin(wire, pmuAddress, PMU_SDA, PMU_SCL)) {
        logger->failure("PMU", "AXP2101 not found");
        initialized = false;
        return false;
    }

    logger->success("PMU", "AXP2101 initialized successfully");
    initialized = true;
    return true;
}

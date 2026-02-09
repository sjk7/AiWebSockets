/**
 * @file TestUtilities.h
 * @brief Common utilities for WebSocket testing
 * 
 * This header contains reusable functions and utilities for WebSocket testing,
 * including test data generation and common test patterns.
 */

#pragma once

#include <vector>
#include <cstdint>
#include <numeric>

namespace WebSocket {

/**
 * @brief Creates test data with sequential byte pattern
 * 
 * Generates a vector of bytes with sequential values starting from 0.
 * This creates a predictable pattern for testing data integrity.
 * 
 * @param dataSize The size of the test data to generate
 * @return std::vector<uint8_t> Test data with pattern 00, 01, 02, ..., FF, 00, 01, ...
 */
inline std::vector<uint8_t> createTestData(size_t dataSize) {
    std::vector<uint8_t> testData(dataSize);
    std::iota(testData.begin(), testData.end(), static_cast<uint8_t>(0));
    return testData;
}

/**
 * @brief Verifies data integrity by comparing received data with expected pattern
 * 
 * @param receivedData The data received from the socket
 * @param expectedSize The expected size of the data
 * @return true if data matches expected pattern, false otherwise
 */
inline bool verifyDataIntegrity(const std::vector<uint8_t>& receivedData, size_t expectedSize) {
    if (receivedData.size() != expectedSize) {
        return false;
    }
    
    for (size_t i = 0; i < expectedSize; i++) {
        if (receivedData[i] != static_cast<uint8_t>(i % 256)) {
            return false;
        }
    }
    
    return true;
}

} // namespace WebSocket

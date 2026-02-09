// WebSocket Protocol Tests - Rigorous and Vigorous Testing
// Comprehensive testing of WebSocket frame creation, generation, and parsing

#include "WebSocket/ErrorCodes.h"
#include "WebSocket/WebSocketProtocol.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// Test helper functions
bool assertEqual(const std::string& expected, const std::string& actual, const std::string& testName) {
    if (expected == actual) {
        std::cout << "✅ " << testName << " - PASSED" << std::endl;
        return true;
    } else {
        std::cout << "❌ " << testName << " - FAILED: Expected '" << expected << "', got '" << actual << "'" << std::endl;
        return false;
    }
}

bool assertEqual(size_t expected, size_t actual, const std::string& testName) {
    if (expected == actual) {
        std::cout << "✅ " << testName << " - PASSED" << std::endl;
        return true;
    } else {
        std::cout << "❌ " << testName << " - FAILED: Expected " << expected << ", got " << actual << std::endl;
        return false;
    }
}

bool assertTrue(bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "✅ " << testName << " - PASSED" << std::endl;
        return true;
    } else {
        std::cout << "❌ " << testName << " - FAILED: Condition was false" << std::endl;
        return false;
    }
}

bool assertFalse(bool condition, const std::string& testName) {
    if (!condition) {
        std::cout << "✅ " << testName << " - PASSED" << std::endl;
        return true;
    } else {
        std::cout << "❌ " << testName << " - FAILED: Condition was true" << std::endl;
        return false;
    }
}

void printHex(const std::vector<uint8_t>& data, const std::string& label) {
    std::cout << label << ": ";
    for (uint8_t byte : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    std::cout << "=== RIGOROUS WebSocket Protocol Tests ===" << std::endl;
    std::cout << "Running comprehensive frame validation tests..." << std::endl;
    
    int totalTests = 0;
    int passedTests = 0;
    
    // Test 1: Text Frame Creation - Empty Payload
    {
        totalTests++;
        std::cout << "\n--- Test 1: Empty Text Frame ---" << std::endl;
        auto frame = nob::WebSocketProtocol::createTextFrame("");
        
        bool result = assertEqual(0, frame.PayloadLength, "Empty text frame payload length") &&
                      assertTrue(frame.Fin, "Empty text frame FIN flag") &&
                      assertFalse(frame.Rsv1, "Empty text frame RSV1 flag") &&
                      assertFalse(frame.Rsv2, "Empty text frame RSV2 flag") &&
                      assertFalse(frame.Rsv3, "Empty text frame RSV3 flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::TEXT), static_cast<size_t>(frame.Opcode), "Empty text frame opcode") &&
                      assertFalse(frame.Masked, "Empty text frame masked flag");
        
        if (result) passedTests++;
    }
    
    // Test 2: Text Frame Creation - Small Payload
    {
        totalTests++;
        std::cout << "\n--- Test 2: Small Text Frame ---" << std::endl;
        std::string testString = "Hello";
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        
        bool result = assertEqual(testString.length(), frame.PayloadLength, "Small text frame payload length") &&
                      assertTrue(frame.Fin, "Small text frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::TEXT), static_cast<size_t>(frame.Opcode), "Small text frame opcode") &&
                      assertFalse(frame.Masked, "Small text frame masked flag");
        
        if (result) passedTests++;
    }
    
    // Test 3: Text Frame Creation - Medium Payload (125 bytes)
    {
        totalTests++;
        std::cout << "\n--- Test 3: Medium Text Frame (125 bytes) ---" << std::endl;
        std::string testString(125, 'A');
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        
        bool result = assertEqual(125, frame.PayloadLength, "Medium text frame payload length") &&
                      assertTrue(frame.Fin, "Medium text frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::TEXT), static_cast<size_t>(frame.Opcode), "Medium text frame opcode");
        
        if (result) passedTests++;
    }
    
    // Test 4: Text Frame Creation - Large Payload (126 bytes - requires 16-bit length)
    {
        totalTests++;
        std::cout << "\n--- Test 4: Large Text Frame (126 bytes) ---" << std::endl;
        std::string testString(126, 'B');
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        
        bool result = assertEqual(126, frame.PayloadLength, "Large text frame payload length") &&
                      assertTrue(frame.Fin, "Large text frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::TEXT), static_cast<size_t>(frame.Opcode), "Large text frame opcode");
        
        if (result) passedTests++;
    }
    
    // Test 5: Text Frame Creation - Very Large Payload (65536 bytes - requires 64-bit length)
    {
        totalTests++;
        std::cout << "\n--- Test 5: Very Large Text Frame (65536 bytes) ---" << std::endl;
        std::string testString(65536, 'C');
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        
        bool result = assertEqual(65536, frame.PayloadLength, "Very large text frame payload length") &&
                      assertTrue(frame.Fin, "Very large text frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::TEXT), static_cast<size_t>(frame.Opcode), "Very large text frame opcode");
        
        if (result) passedTests++;
    }
    
    // Test 6: Binary Frame Creation
    {
        totalTests++;
        std::cout << "\n--- Test 6: Binary Frame ---" << std::endl;
        std::vector<uint8_t> binaryData = {0x01, 0x02, 0x03, 0x04, 0x05};
        auto frame = nob::WebSocketProtocol::createBinaryFrame(binaryData);
        
        bool result = assertEqual(binaryData.size(), frame.PayloadLength, "Binary frame payload length") &&
                      assertTrue(frame.Fin, "Binary frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::BINARY), static_cast<size_t>(frame.Opcode), "Binary frame opcode") &&
                      assertFalse(frame.Masked, "Binary frame masked flag");
        
        if (result) passedTests++;
    }
    
    // Test 7: Ping Frame Creation
    {
        totalTests++;
        std::cout << "\n--- Test 7: Ping Frame ---" << std::endl;
        auto frame = nob::WebSocketProtocol::createPingFrame();
        
        bool result = assertEqual(0, frame.PayloadLength, "Ping frame payload length") &&
                      assertTrue(frame.Fin, "Ping frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::PING), static_cast<size_t>(frame.Opcode), "Ping frame opcode") &&
                      assertFalse(frame.Masked, "Ping frame masked flag");
        
        if (result) passedTests++;
    }
    
    // Test 8: Pong Frame Creation
    {
        totalTests++;
        std::cout << "\n--- Test 8: Pong Frame ---" << std::endl;
        auto frame = nob::WebSocketProtocol::createPongFrame();
        
        bool result = assertEqual(0, frame.PayloadLength, "Pong frame payload length") &&
                      assertTrue(frame.Fin, "Pong frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::PONG), static_cast<size_t>(frame.Opcode), "Pong frame opcode") &&
                      assertFalse(frame.Masked, "Pong frame masked flag");
        
        if (result) passedTests++;
    }
    
    // Test 9: Close Frame Creation - Default
    {
        totalTests++;
        std::cout << "\n--- Test 9: Close Frame (Default) ---" << std::endl;
        auto frame = nob::WebSocketProtocol::createCloseFrame();
        
        bool result = assertEqual(2, frame.PayloadLength, "Close frame payload length (default)") &&
                      assertTrue(frame.Fin, "Close frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::CLOSE), static_cast<size_t>(frame.Opcode), "Close frame opcode") &&
                      assertFalse(frame.Masked, "Close frame masked flag");
        
        if (result) passedTests++;
    }
    
    // Test 10: Close Frame Creation - Custom Code and Reason
    {
        totalTests++;
        std::cout << "\n--- Test 10: Close Frame (Custom) ---" << std::endl;
        std::string reason = "Normal closure";
        auto frame = nob::WebSocketProtocol::createCloseFrame(1000, reason);
        
        bool result = assertEqual(2 + reason.length(), frame.PayloadLength, "Close frame payload length (custom)") &&
                      assertTrue(frame.Fin, "Close frame FIN flag") &&
                      assertEqual(static_cast<size_t>(nob::websocketOpcode::CLOSE), static_cast<size_t>(frame.Opcode), "Close frame opcode") &&
                      assertFalse(frame.Masked, "Close frame masked flag");
        
        if (result) passedTests++;
    }
    
    // Test 11: Frame Generation - Small Frame
    {
        totalTests++;
        std::cout << "\n--- Test 11: Frame Generation (Small) ---" << std::endl;
        std::string testString = "Test";
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        auto frameData = nob::WebSocketProtocol::generateFrame(frame);
        
        bool result = assertTrue(!frameData.empty(), "Frame generation produces data") &&
                      assertEqual(testString.length() + 2, frameData.size(), "Small frame data size"); // 2 bytes header + payload
        
        if (result) {
            printHex(frameData, "Generated small frame");
            passedTests++;
        }
    }
    
    // Test 12: Frame Generation - Medium Frame (125 bytes)
    {
        totalTests++;
        std::cout << "\n--- Test 12: Frame Generation (Medium) ---" << std::endl;
        std::string testString(125, 'M');
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        auto frameData = nob::WebSocketProtocol::generateFrame(frame);
        
        bool result = assertTrue(!frameData.empty(), "Medium frame generation produces data") &&
                      assertEqual(125 + 2, frameData.size(), "Medium frame data size"); // 2 bytes header + payload
        
        if (result) {
            std::cout << "Generated medium frame size: " << frameData.size() << " bytes" << std::endl;
            passedTests++;
        }
    }
    
    // Test 13: Frame Generation - Large Frame (126 bytes)
    {
        totalTests++;
        std::cout << "\n--- Test 13: Frame Generation (Large) ---" << std::endl;
        std::string testString(126, 'L');
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        auto frameData = nob::WebSocketProtocol::generateFrame(frame);
        
        bool result = assertTrue(!frameData.empty(), "Large frame generation produces data") &&
                      assertEqual(126 + 4, frameData.size(), "Large frame data size"); // 4 bytes header + payload (2 extra for 16-bit length)
        
        if (result) {
            std::cout << "Generated large frame size: " << frameData.size() << " bytes" << std::endl;
            passedTests++;
        }
    }
    
    // Test 14: Frame Generation - Very Large Frame (65536 bytes)
    {
        totalTests++;
        std::cout << "\n--- Test 14: Frame Generation (Very Large) ---" << std::endl;
        std::string testString(65536, 'V');
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        auto frameData = nob::WebSocketProtocol::generateFrame(frame);
        
        bool result = assertTrue(!frameData.empty(), "Very large frame generation produces data") &&
                      assertEqual(65536 + 10, frameData.size(), "Very large frame data size"); // 10 bytes header + payload (8 extra for 64-bit length)
        
        if (result) {
            std::cout << "Generated very large frame size: " << frameData.size() << " bytes" << std::endl;
            passedTests++;
        }
    }
    
    // Test 15: Frame Parsing - Round Trip Test
    {
        totalTests++;
        std::cout << "\n--- Test 15: Frame Parsing Round Trip ---" << std::endl;
        std::string originalMessage = "Round trip test message with special chars: !@#$%^&*()";
        
        // Create frame
        auto originalFrame = nob::WebSocketProtocol::createTextFrame(originalMessage);
        
        // Generate frame data
        auto frameData = nob::WebSocketProtocol::generateFrame(originalFrame);
        
        // Parse frame back
        nob::WebSocketFrame parsedFrame;
        size_t bytesConsumed = 0;
        auto parseResult = nob::WebSocketProtocol::parseFrame(frameData, parsedFrame, bytesConsumed);
        
        bool result = assertTrue(parseResult.isSuccess(), "Frame parsing succeeds") &&
                      assertEqual(originalFrame.PayloadLength, parsedFrame.PayloadLength, "Round trip payload length") &&
                      assertEqual(static_cast<size_t>(originalFrame.Opcode), static_cast<size_t>(parsedFrame.Opcode), "Round trip opcode") &&
                      assertEqual(originalFrame.Fin, parsedFrame.Fin, "Round trip FIN flag") &&
                      assertEqual(originalFrame.Masked, parsedFrame.Masked, "Round trip masked flag") &&
                      assertTrue(bytesConsumed > 0, "Round trip bytes consumed") &&
                      assertEqual(frameData.size(), bytesConsumed, "Round trip consumed all bytes");
        
        if (result) {
            std::cout << "Original message: '" << originalMessage << "'" << std::endl;
            std::cout << "Parsed payload length: " << parsedFrame.PayloadLength << std::endl;
            std::cout << "Bytes consumed: " << bytesConsumed << "/" << frameData.size() << std::endl;
            passedTests++;
        }
    }
    
    // Test 16: Invalid Opcode Detection
    {
        totalTests++;
        std::cout << "\n--- Test 16: Invalid Opcode Detection ---" << std::endl;
        
        // Test valid opcodes
        bool validText = nob::WebSocketProtocol::isValidOpcode(nob::websocketOpcode::TEXT);
        bool validBinary = nob::WebSocketProtocol::isValidOpcode(nob::websocketOpcode::BINARY);
        bool validClose = nob::WebSocketProtocol::isValidOpcode(nob::websocketOpcode::CLOSE);
        bool validPing = nob::WebSocketProtocol::isValidOpcode(nob::websocketOpcode::PING);
        bool validPong = nob::WebSocketProtocol::isValidOpcode(nob::websocketOpcode::PONG);
        
        // Test invalid opcode (assuming 0x7 is invalid)
        bool invalidOpcode = nob::WebSocketProtocol::isValidOpcode(static_cast<nob::websocketOpcode>(0x7));
        
        bool result = assertTrue(validText, "TEXT opcode is valid") &&
                      assertTrue(validBinary, "BINARY opcode is valid") &&
                      assertTrue(validClose, "CLOSE opcode is valid") &&
                      assertTrue(validPing, "PING opcode is valid") &&
                      assertTrue(validPong, "PONG opcode is valid") &&
                      assertFalse(invalidOpcode, "Invalid opcode (0x7) should be false");
        
        if (result) {
            std::cout << "Valid opcode tests: PASSED" << std::endl;
            std::cout << "Invalid opcode test: PASSED" << std::endl;
            passedTests++;
        }
    }
    
    // Test 17: UTF-8 Validation
    {
        totalTests++;
        std::cout << "\n--- Test 17: UTF-8 Validation ---" << std::endl;
        
        // Valid UTF-8
        std::vector<uint8_t> validUTF8 = {'H', 'e', 'l', 'l', 'o', 0xE2, 0x98, 0x83}; // "Hello☃"
        bool validResult = nob::WebSocketProtocol::isValidUTF8(validUTF8);
        
        // Invalid UTF-8 (truncated sequence)
        std::vector<uint8_t> invalidUTF8 = {'H', 'e', 'l', 'l', 'o', 0xE2}; // Truncated
        bool invalidResult = nob::WebSocketProtocol::isValidUTF8(invalidUTF8);
        
        bool result = assertTrue(validResult, "Valid UTF-8 should pass") &&
                      assertFalse(invalidResult, "Invalid UTF-8 should fail");
        
        if (result) {
            std::cout << "Valid UTF-8 test: PASSED" << std::endl;
            std::cout << "Invalid UTF-8 test: PASSED" << std::endl;
            passedTests++;
        }
    }
    
    // Test 18: Frame Header Validation
    {
        totalTests++;
        std::cout << "\n--- Test 18: Frame Header Validation ---" << std::endl;
        
        std::string testMessage = "Header validation test";
        auto frame = nob::WebSocketProtocol::createTextFrame(testMessage);
        auto frameData = nob::WebSocketProtocol::generateFrame(frame);
        
        // Check header structure
        bool result = assertTrue(frameData.size() >= 2, "Frame has minimum header size");
        
        if (result) {
            // Check first byte: FIN + RSV + Opcode
            uint8_t firstByte = frameData[0];
            bool finSet = (firstByte & 0x80) != 0;
            bool rsvClear = (firstByte & 0x70) == 0;
            uint8_t opcode = firstByte & 0x0F;
            
            result = result && assertTrue(finSet, "FIN bit is set in header") &&
                           assertTrue(rsvClear, "RSV bits are clear in header") &&
                           assertEqual(static_cast<uint8_t>(nob::websocketOpcode::TEXT), opcode, "Opcode matches in header");
            
            // Check second byte: MASK + Payload length
            uint8_t secondByte = frameData[1];
            bool maskClear = (secondByte & 0x80) == 0;
            uint8_t payloadLen = secondByte & 0x7F;
            
            result = result && assertTrue(maskClear, "Mask bit is clear in header") &&
                           assertEqual(testMessage.length(), payloadLen, "Payload length matches in header");
            
            if (result) {
                printHex(frameData, "Frame header bytes");
                std::cout << "FIN bit: " << (finSet ? "SET" : "CLEAR") << std::endl;
                std::cout << "RSV bits: " << (rsvClear ? "CLEAR" : "SET") << std::endl;
                std::cout << "Opcode: 0x" << std::hex << (int)opcode << std::dec << std::endl;
                std::cout << "Mask bit: " << (maskClear ? "CLEAR" : "SET") << std::endl;
                std::cout << "Payload length: " << (int)payloadLen << std::endl;
            }
        }
        
        if (result) passedTests++;
    }
    
    // Test 19: Multiple Frame Generation and Parsing
    {
        totalTests++;
        std::cout << "\n--- Test 19: Multiple Frame Processing ---" << std::endl;
        
        std::vector<std::string> messages = {
            "Message 1",
            "A longer message 2 with more content",
            std::string(200, 'X'), // 200 character message
            "Final message"
        };
        
        std::vector<std::vector<uint8_t>> allFrameData;
        
        // Generate multiple frames
        for (const auto& message : messages) {
            auto frame = nob::WebSocketProtocol::createTextFrame(message);
            auto frameData = nob::WebSocketProtocol::generateFrame(frame);
            allFrameData.push_back(frameData);
        }
        
        // Concatenate all frame data
        std::vector<uint8_t> combinedData;
        for (const auto& frameData : allFrameData) {
            combinedData.insert(combinedData.end(), frameData.begin(), frameData.end());
        }
        
        // Parse all frames sequentially
        bool result = true;
        size_t offset = 0;
        for (size_t i = 0; i < messages.size() && result; ++i) {
            nob::WebSocketFrame parsedFrame;
            size_t bytesConsumed = 0;
            
            std::vector<uint8_t> remainingData(combinedData.begin() + offset, combinedData.end());
            auto parseResult = nob::WebSocketProtocol::parseFrame(remainingData, parsedFrame, bytesConsumed);
            
            result = result && assertTrue(parseResult.isSuccess(), "Frame " + std::to_string(i+1) + " parsing succeeds") &&
                              assertEqual(messages[i].length(), parsedFrame.PayloadLength, "Frame " + std::to_string(i+1) + " payload length") &&
                              assertEqual(static_cast<size_t>(nob::websocketOpcode::TEXT), static_cast<size_t>(parsedFrame.Opcode), "Frame " + std::to_string(i+1) + " opcode") &&
                              assertTrue(bytesConsumed > 0, "Frame " + std::to_string(i+1) + " consumes bytes");
            
            offset += bytesConsumed;
        }
        
        result = result && assertEqual(combinedData.size(), offset, "All bytes consumed in multi-frame test");
        
        if (result) {
            std::cout << "Successfully processed " << messages.size() << " frames" << std::endl;
            std::cout << "Total data size: " << combinedData.size() << " bytes" << std::endl;
            passedTests++;
        }
    }
    
    // Test 20: Edge Case - Maximum Small Frame (125 bytes)
    {
        totalTests++;
        std::cout << "\n--- Test 20: Edge Case - Maximum Small Frame ---" << std::endl;
        
        std::string testString(125, 'E'); // Exactly 125 bytes
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        auto frameData = nob::WebSocketProtocol::generateFrame(frame);
        
        // Should use 2-byte header (no extended length)
        bool result = assertEqual(125 + 2, frameData.size(), "Max small frame uses 2-byte header") &&
                      assertTrue(frameData.size() <= 127, "Frame size indicates no extended length");
        
        if (result) {
            std::cout << "Frame size: " << frameData.size() << " bytes (expected 127)" << std::endl;
            passedTests++;
        }
    }
    
    // Test 21: Edge Case - Minimum Large Frame (126 bytes)
    {
        totalTests++;
        std::cout << "\n--- Test 21: Edge Case - Minimum Large Frame ---" << std::endl;
        
        std::string testString(126, 'F'); // Exactly 126 bytes
        auto frame = nob::WebSocketProtocol::createTextFrame(testString);
        auto frameData = nob::WebSocketProtocol::generateFrame(frame);
        
        // Should use 4-byte header (16-bit extended length)
        bool result = assertEqual(126 + 4, frameData.size(), "Min large frame uses 4-byte header") &&
                      assertTrue(frameData.size() >= 130, "Frame size indicates extended length");
        
        if (result) {
            std::cout << "Frame size: " << frameData.size() << " bytes (expected 130)" << std::endl;
            // Check that the second byte has the 126 marker
            if (frameData.size() >= 2) {
                uint8_t lengthByte = frameData[1];
                bool usesExtendedLength = (lengthByte & 0x7F) == 126;
                result = result && assertTrue(usesExtendedLength, "Uses 126 extended length marker");
                std::cout << "Extended length marker: " << (usesExtendedLength ? "SET" : "CLEAR") << std::endl;
            }
            passedTests++;
        }
    }
    
    // Final Results
    std::cout << "\n=== RIGOROUS TEST RESULTS ===" << std::endl;
    std::cout << "Total tests: " << totalTests << std::endl;
    std::cout << "Passed: " << passedTests << std::endl;
    std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1) 
              << (totalTests > 0 ? (100.0 * passedTests / totalTests) : 0.0) << "%" << std::endl;
    
    if (passedTests == totalTests) {
        std::cout << "\n🎉 ALL RIGOROUS TESTS PASSED! WebSocket protocol implementation is robust!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some tests failed. Review the implementation." << std::endl;
        return 1;
    }
}

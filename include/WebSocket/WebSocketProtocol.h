#pragma once

#include "Types.h"
#include "ErrorCodes.h"
#include <vector>
#include <string>

namespace WebSocket {

/**
 * @brief WebSocket Protocol Implementation
 * 
 * This class handles the WebSocket protocol according to RFC 6455.
 * It provides functionality for handshake processing, frame parsing,
 * and frame generation.
 */
class WebSocketProtocol {
public:
    // Handshake methods
    static Result validateHandshakeRequest(const std::string& request, HandshakeInfo& info);
    static std::string generateHandshakeResponse(const HandshakeInfo& info);
    static std::string generateWebSocketKey(const std::string& clientKey);
    
    // Subprotocol negotiation
    static std::string negotiateSubProtocol(const std::vector<std::string>& clientProtocols, 
                                          const std::vector<std::string>& serverProtocols);
    
    // Frame parsing methods
    static Result parseFrame(const std::vector<uint8_t>& data, WebSocketFrame& frame, size_t& bytesConsumed);
    static std::vector<uint8_t> generateFrame(const WebSocketFrame& frame);
    
    // Message utilities
    static WebSocketFrame createTextFrame(const std::string& text, bool fin = true);
    static WebSocketFrame createBinaryFrame(const std::vector<uint8_t>& data, bool fin = true);
    static WebSocketFrame createPingFrame(const std::vector<uint8_t>& data = {});
    static WebSocketFrame createPongFrame(const std::vector<uint8_t>& data = {});
    static WebSocketFrame createCloseFrame(uint16_t code = 1000, const std::string& reason = "");
    
    // Validation methods
    static bool isValidOpcode(websocketOpcode opcode);
    static bool isValidUTF8(const std::vector<uint8_t>& data);
    
private:
    static std::string base64Encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> base64Decode(const std::string& data);
    static std::string sha1Hash(const std::string& input);
};

} // namespace WebSocket

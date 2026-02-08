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
    static Result ValidateHandshakeRequest(const std::string& request, HandshakeInfo& info);
    static std::string GenerateHandshakeResponse(const HandshakeInfo& info);
    static std::string GenerateWebSocketKey(const std::string& clientKey);
    
    // Subprotocol negotiation
    static std::string NegotiateSubProtocol(const std::vector<std::string>& clientProtocols, 
                                          const std::vector<std::string>& serverProtocols);
    
    // Frame parsing methods
    static Result ParseFrame(const std::vector<uint8_t>& data, WebSocketFrame& frame, size_t& bytesConsumed);
    static std::vector<uint8_t> GenerateFrame(const WebSocketFrame& frame);
    
    // Message utilities
    static WebSocketFrame CreateTextFrame(const std::string& text, bool fin = true);
    static WebSocketFrame CreateBinaryFrame(const std::vector<uint8_t>& data, bool fin = true);
    static WebSocketFrame CreatePingFrame(const std::vector<uint8_t>& data = {});
    static WebSocketFrame CreatePongFrame(const std::vector<uint8_t>& data = {});
    static WebSocketFrame CreateCloseFrame(uint16_t code = 1000, const std::string& reason = "");
    
    // Validation methods
    static bool IsValidOpcode(WEBSOCKET_OPCODE opcode);
    static bool IsValidUTF8(const std::vector<uint8_t>& data);
    
private:
    static std::string Base64Encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> Base64Decode(const std::string& data);
    static std::string SHA1Hash(const std::string& input);
};

} // namespace WebSocket

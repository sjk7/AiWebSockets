#pragma once

#include <functional>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include "ErrorCodes.h"

namespace WebSocket {

// Forward declarations
class Socket;
class WebSocketConnection;

// Enum classes with proper casing
enum class SOCKET_TYPE {
    TCP,
    UDP
};

enum class SOCKET_FAMILY {
    IPV4,
    IPV6
};

enum class WEBSOCKET_OPCODE {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

enum class WEBSOCKET_STATE {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED
};

// Structs for WebSocket data
struct WebSocketFrame {
    bool Fin;
    bool Rsv1;
    bool Rsv2;
    bool Rsv3;
    WEBSOCKET_OPCODE Opcode;
    bool Masked;
    uint64_t PayloadLength;
    std::vector<uint8_t> MaskingKey;
    std::vector<uint8_t> PayloadData;
};

struct WebSocketMessage {
    WEBSOCKET_OPCODE Opcode;
    std::vector<uint8_t> Data;
    
    bool IsText() const { return Opcode == WEBSOCKET_OPCODE::TEXT; }
    bool IsBinary() const { return Opcode == WEBSOCKET_OPCODE::BINARY; }
    bool IsClose() const { return Opcode == WEBSOCKET_OPCODE::CLOSE; }
    bool IsPing() const { return Opcode == WEBSOCKET_OPCODE::PING; }
    bool IsPong() const { return Opcode == WEBSOCKET_OPCODE::PONG; }
    
    std::string AsText() const {
        return IsText() ? std::string(Data.begin(), Data.end()) : std::string();
    }
};

// Callback types
using ConnectionCallback = std::function<void(std::shared_ptr<WebSocketConnection>)>;
using MessageCallback = std::function<void(std::shared_ptr<WebSocketConnection>, const WebSocketMessage&)>;
using CloseCallback = std::function<void(std::shared_ptr<WebSocketConnection>, uint16_t code, const std::string& reason)>;
using ErrorCallback = std::function<void(std::shared_ptr<WebSocketConnection>, const Result& result)>;

// Configuration structs
struct ServerConfig {
    uint16_t Port = 8080;
    std::string Host = "0.0.0.0";
    int MaxConnections = 1000;
    size_t MaxMessageSize = 16 * 1024 * 1024; // 16MB
    bool EnableCompression = false;
    std::vector<std::string> AllowedOrigins;
    std::string SubProtocol;
};

struct HandshakeInfo {
    std::string Host;
    std::string Origin;
    std::string Key;
    std::string Version;
    std::string Protocol;  // Selected protocol (single)
    std::vector<std::string> Protocols;  // Requested protocols (multiple)
    std::vector<std::string> Extensions;
    std::vector<std::pair<std::string, std::string>> Headers;
};

} // namespace WebSocket

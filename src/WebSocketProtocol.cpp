#include "WebSocket/WebSocketProtocol.h"
#include <cstdio>
#include <cstring>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <openssl/sha.h>
#include <openssl/rand.h>
#endif

namespace nob {

// Base64 encoding table
static const char BASE64_CHARS[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string WebSocketProtocol::base64Encode(const std::vector<uint8_t>& data) {
    std::string result;
    int val = 0, valb = -6;
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        result.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (result.size() % 4) {
        result.push_back('=');
    }
    return result;
}

std::vector<uint8_t> WebSocketProtocol::base64Decode(const std::string& input) {
    std::vector<uint8_t> result;
    int val = 0, valb = -8;
    for (char c : input) {
        if (c == '=') break;
        const char* pos = std::strchr(BASE64_CHARS, c);
        if (!pos) continue;
        val = (val << 6) + static_cast<int>(pos - BASE64_CHARS);
        valb += 6;
        if (valb >= 0) {
            result.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return result;
}

std::string WebSocketProtocol::sha1Hash(const std::string& input) {
#ifdef _WIN32
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[20];
    DWORD hashLen = 20;
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0)) {
        return "";
    }
    
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptHashData(hHash, (BYTE*)input.c_str(), (DWORD)input.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    std::string result;
    for (DWORD i = 0; i < hashLen; i++) {
        result.push_back(hash[i]);
    }
    return result;
#else
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)input.c_str(), input.length(), hash);
    return std::string((char*)hash, SHA_DIGEST_LENGTH);
#endif
}

Result WebSocketProtocol::validateHandshakeRequest(const std::string& request, HandshakeInfo& info) {
    // Parse request line
    size_t lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) {
        return Result(ErrorCode::websocketHandshakeFailed, "Invalid HTTP request format");
    }
    
    std::string requestLine = request.substr(0, lineEnd);
    std::stringstream ss(requestLine);
    std::string method, path, version;
    
    if (!(ss >> method >> path >> version)) {
        return Result(ErrorCode::websocketHandshakeFailed, "Invalid request line");
    }
    
    if (method != "GET") {
        return Result(ErrorCode::websocketHandshakeFailed, "Only GET method allowed");
    }
    
    if (version != "HTTP/1.1") {
        return Result(ErrorCode::websocketHandshakeFailed, "Only HTTP/1.1 supported");
    }
    
    // Parse headers
    size_t pos = lineEnd + 2; // Skip \r\n
    bool hasUpgrade = false;
    bool hasConnection = false;
    bool hasKey = false;
    bool hasVersion = false;
    
    while (pos < request.length()) {
        size_t headerEnd = request.find("\r\n", pos);
        if (headerEnd == std::string::npos) break;
        
        if (headerEnd == pos) break; // Empty line = end of headers
        
        std::string headerLine = request.substr(pos, headerEnd - pos);
        size_t colonPos = headerLine.find(':');
        
        if (colonPos != std::string::npos) {
            std::string name = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            
            // Trim whitespace
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0, 1);
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
            
            // Convert to lowercase for comparison
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName == "upgrade") {
                if (value != "websocket") {
                    return Result(ErrorCode::websocketHandshakeFailed, "Invalid Upgrade value");
                }
                hasUpgrade = true;
            }
            else if (lowerName == "connection") {
                // Connection header can contain multiple values (e.g., "Upgrade, keep-alive")
                if (value.find("Upgrade") == std::string::npos) {
                    return Result(ErrorCode::websocketHandshakeFailed, "Connection must include Upgrade");
                }
                hasConnection = true;
            }
            else if (lowerName == "sec-websocket-key") {
                if (value.empty() || value.length() < 16) {
                    return Result(ErrorCode::websocketHandshakeFailed, "Invalid Sec-WebSocket-Key");
                }
                info.Key = value;
                hasKey = true;
            }
            else if (lowerName == "sec-websocket-version") {
                if (value != "13") {
                    return Result(ErrorCode::websocketHandshakeFailed, "Unsupported WebSocket version");
                }
                info.Version = value;
                hasVersion = true;
            }
            else if (lowerName == "origin") {
                info.Origin = value;
            }
            else if (lowerName == "host") {
                info.Host = value;
            }
            else if (lowerName == "sec-websocket-protocol") {
                // Parse comma-separated protocols
                std::stringstream protocols(value);
                std::string protocol;
                while (std::getline(protocols, protocol, ',')) {
                    // Trim whitespace
                    while (!protocol.empty() && (protocol[0] == ' ' || protocol[0] == '\t')) protocol.erase(0, 1);
                    while (!protocol.empty() && (protocol.back() == ' ' || protocol.back() == '\t')) protocol.pop_back();
                    if (!protocol.empty()) {
                        info.Protocols.push_back(protocol);
                    }
                }
            }
            else if (lowerName == "sec-websocket-extensions") {
                // Parse extensions (simplified)
                std::stringstream extensions(value);
                std::string extension;
                while (std::getline(extensions, extension, ',')) {
                    // Trim whitespace
                    while (!extension.empty() && (extension[0] == ' ' || extension[0] == '\t')) extension.erase(0, 1);
                    while (!extension.empty() && (extension.back() == ' ' || extension.back() == '\t')) extension.pop_back();
                    if (!extension.empty()) {
                        info.Extensions.push_back(extension);
                    }
                }
            }
            
            // Store all headers for reference
            info.Headers.emplace_back(name, value);
        }
        
        pos = headerEnd + 2; // Skip \r\n
    }
    
    // Validate required headers
    if (!hasUpgrade) {
        return Result(ErrorCode::websocketHandshakeFailed, "Missing Upgrade header");
    }
    
    if (!hasConnection) {
        return Result(ErrorCode::websocketHandshakeFailed, "Missing Connection header");
    }
    
    if (!hasKey) {
        return Result(ErrorCode::websocketHandshakeFailed, "Missing Sec-WebSocket-Key header");
    }
    
    if (!hasVersion) {
        return Result(ErrorCode::websocketHandshakeFailed, "Missing Sec-WebSocket-Version header");
    }
    
    return Result();
}

std::string WebSocketProtocol::generateHandshakeResponse(const HandshakeInfo& info) {
    std::string acceptKey = generateWebSocketKey(info.Key);
    
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n";
    
    // Add selected subprotocol if negotiated
    if (!info.Protocol.empty()) {
        response << "Sec-WebSocket-Protocol: " << info.Protocol << "\r\n";
    }
    
    // Add supported extensions (currently none, but framework ready)
    if (!info.Extensions.empty()) {
        // For now, we don't support any extensions, so we don't include this header
        // In a full implementation, we'd negotiate extensions here
    }
    
    response << "\r\n";
    
    return response.str();
}

std::string WebSocketProtocol::generateWebSocketKey(const std::string& clientKey) {
    std::string magicString = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string hash = sha1Hash(magicString);
    return base64Encode(std::vector<uint8_t>(hash.begin(), hash.end()));
}

std::string WebSocketProtocol::negotiateSubProtocol(const std::vector<std::string>& clientProtocols, 
                                                   const std::vector<std::string>& serverProtocols) {
    // Find the first protocol that both client and server support
    // RFC 6455: Use the first protocol in the client's list that the server also supports
    for (const std::string& clientProto : clientProtocols) {
        for (const std::string& serverProto : serverProtocols) {
            if (clientProto == serverProto) {
                return clientProto; // Return the client's protocol name (should be same)
            }
        }
    }
    return ""; // No common protocol found
}

Result WebSocketProtocol::parseFrame(const std::vector<uint8_t>& data, WebSocketFrame& frame, size_t& bytesConsumed) {
    if (data.size() < 2) {
        return Result(ErrorCode::websocketFrameParseFailed, "Frame too short");
    }
    
    // Parse first byte
    frame.Fin = (data[0] & 0x80) != 0;
    frame.Rsv1 = (data[0] & 0x40) != 0;
    frame.Rsv2 = (data[0] & 0x20) != 0;
    frame.Rsv3 = (data[0] & 0x10) != 0;
    frame.Opcode = static_cast<websocketOpcode>(data[0] & 0x0F);
    
    // Parse second byte
    frame.Masked = (data[1] & 0x80) != 0;
    uint8_t payloadLen1 = data[1] & 0x7F;
    
    size_t offset = 2;
    
    // Parse extended payload length
    if (payloadLen1 == 126) {
        if (data.size() < offset + 2) {
            return Result(ErrorCode::websocketFrameParseFailed, "Incomplete extended payload length");
        }
        frame.PayloadLength = (static_cast<uint64_t>(data[offset]) << 8) | data[offset + 1];
        offset += 2;
    } else if (payloadLen1 == 127) {
        if (data.size() < offset + 8) {
            return Result(ErrorCode::websocketFrameParseFailed, "Incomplete extended payload length");
        }
        frame.PayloadLength = 0;
        for (int i = 0; i < 8; i++) {
            frame.PayloadLength = (frame.PayloadLength << 8) | data[offset + i];
        }
        offset += 8;
    } else {
        frame.PayloadLength = payloadLen1;
    }
    
    // Parse masking key (if present)
    if (frame.Masked) {
        if (data.size() < offset + 4) {
            return Result(ErrorCode::websocketFrameParseFailed, "Incomplete masking key");
        }
        frame.MaskingKey.assign(data.begin() + offset, data.begin() + offset + 4);
        offset += 4;
    } else {
        frame.MaskingKey.clear();
    }
    
    // Parse payload data
    if (data.size() < offset + frame.PayloadLength) {
        return Result(ErrorCode::websocketFrameParseFailed, "Incomplete payload data");
    }
    
    frame.PayloadData.assign(data.begin() + offset, data.begin() + offset + frame.PayloadLength);
    
    // Unmask payload if necessary
    if (frame.Masked && !frame.MaskingKey.empty()) {
        for (size_t i = 0; i < frame.PayloadData.size(); i++) {
            frame.PayloadData[i] ^= frame.MaskingKey[i % 4];
        }
    }
    
    bytesConsumed = offset + frame.PayloadLength;
    return Result();
}

std::vector<uint8_t> WebSocketProtocol::generateFrame(const WebSocketFrame& frame) {
    std::vector<uint8_t> result;
    
    // First byte
    uint8_t firstByte = 0;
    if (frame.Fin) firstByte |= 0x80;
    if (frame.Rsv1) firstByte |= 0x40;
    if (frame.Rsv2) firstByte |= 0x20;
    if (frame.Rsv3) firstByte |= 0x10;
    firstByte |= static_cast<uint8_t>(frame.Opcode) & 0x0F;
    result.push_back(firstByte);
    
    // Second byte and payload length
    uint8_t secondByte = frame.Masked ? 0x80 : 0x00;
    if (frame.PayloadLength < 126) {
        secondByte |= static_cast<uint8_t>(frame.PayloadLength);
        result.push_back(secondByte);
    } else if (frame.PayloadLength < 65536) {
        secondByte |= 126;
        result.push_back(secondByte);
        result.push_back(static_cast<uint8_t>((frame.PayloadLength >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(frame.PayloadLength & 0xFF));
    } else {
        secondByte |= 127;
        result.push_back(secondByte);
        for (int i = 7; i >= 0; i--) {
            result.push_back(static_cast<uint8_t>((frame.PayloadLength >> (i * 8)) & 0xFF));
        }
    }
    
    // Masking key (if needed)
    if (frame.Masked) {
        if (frame.MaskingKey.size() != 4) {
            // Generate random masking key if not provided
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(0, 255);
            for (int i = 0; i < 4; i++) {
                result.push_back(static_cast<uint8_t>(dis(gen)));
            }
        } else {
            result.insert(result.end(), frame.MaskingKey.begin(), frame.MaskingKey.end());
        }
    }
    
    // Payload data
    result.insert(result.end(), frame.PayloadData.begin(), frame.PayloadData.end());
    
    return result;
}

WebSocketFrame WebSocketProtocol::createTextFrame(const std::string& text, bool fin) {
    WebSocketFrame frame;
    frame.Fin = fin;
    frame.Rsv1 = frame.Rsv2 = frame.Rsv3 = false;
    frame.Opcode = websocketOpcode::TEXT;
    frame.Masked = false;
    frame.PayloadLength = text.length();
    frame.PayloadData.assign(text.begin(), text.end());
    return frame;
}

WebSocketFrame WebSocketProtocol::createBinaryFrame(const std::vector<uint8_t>& data, bool fin) {
    WebSocketFrame frame;
    frame.Fin = fin;
    frame.Rsv1 = frame.Rsv2 = frame.Rsv3 = false;
    frame.Opcode = websocketOpcode::BINARY;
    frame.Masked = false;
    frame.PayloadLength = data.size();
    frame.PayloadData = data;
    return frame;
}

WebSocketFrame WebSocketProtocol::createPingFrame(const std::vector<uint8_t>& data) {
    WebSocketFrame frame;
    frame.Fin = true;
    frame.Rsv1 = frame.Rsv2 = frame.Rsv3 = false;
    frame.Opcode = websocketOpcode::PING;
    frame.Masked = false;
    frame.PayloadLength = data.size();
    frame.PayloadData = data;
    return frame;
}

WebSocketFrame WebSocketProtocol::createPongFrame(const std::vector<uint8_t>& data) {
    WebSocketFrame frame;
    frame.Fin = true;
    frame.Rsv1 = frame.Rsv2 = frame.Rsv3 = false;
    frame.Opcode = websocketOpcode::PONG;
    frame.Masked = false;
    frame.PayloadLength = data.size();
    frame.PayloadData = data;
    return frame;
}

WebSocketFrame WebSocketProtocol::createCloseFrame(uint16_t code, const std::string& reason) {
    WebSocketFrame frame;
    frame.Fin = true;
    frame.Rsv1 = frame.Rsv2 = frame.Rsv3 = false;
    frame.Opcode = websocketOpcode::CLOSE;
    frame.Masked = false;
    
    frame.PayloadLength = 2 + reason.length();
    frame.PayloadData.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
    frame.PayloadData.push_back(static_cast<uint8_t>(code & 0xFF));
    frame.PayloadData.insert(frame.PayloadData.end(), reason.begin(), reason.end());
    
    return frame;
}

bool WebSocketProtocol::isValidOpcode(websocketOpcode opcode) {
    switch (opcode) {
        case websocketOpcode::CONTINUATION:
        case websocketOpcode::TEXT:
        case websocketOpcode::BINARY:
        case websocketOpcode::CLOSE:
        case websocketOpcode::PING:
        case websocketOpcode::PONG:
            return true;
        default:
            return false;
    }
}

bool WebSocketProtocol::isValidUTF8(const std::vector<uint8_t>& data) {
    // Simplified UTF-8 validation
    // In a full implementation, this would be more thorough
    for (size_t i = 0; i < data.size(); ) {
        uint8_t c = data[i];
        if (c < 0x80) {
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= data.size()) return false;
            if ((data[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= data.size()) return false;
            if ((data[i + 1] & 0xC0) != 0x80) return false;
            if ((data[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= data.size()) return false;
            if ((data[i + 1] & 0xC0) != 0x80) return false;
            if ((data[i + 2] & 0xC0) != 0x80) return false;
            if ((data[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

} // namespace nob

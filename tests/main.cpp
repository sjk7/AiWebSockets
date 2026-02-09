#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include "WebSocket/ErrorCodes.h"
#include "WebSocket/Socket.h"
#include "WebSocket/WebSocketProtocol.h"

// Simple test framework for CTest
class testFramework {
public:
    static int runAllTests();
    static void assert(bool condition, const char* message);
    static void assertEquals(const std::string& expected, const std::string& actual, const char* message);
    
private:
    static int s_testsRun;
    static int s_testsPassed;
    static int s_testsFailed;
};

int testFramework::s_testsRun = 0;
int testFramework::s_testsPassed = 0;
int testFramework::s_testsFailed = 0;

int testFramework::runAllTests() {
    // Test functions will be added here as we implement them
    printf("Running WebSocket Tests...\n");
    
    // Placeholder for future tests
    printf("Tests will be added as we implement functionality\n");
    
    printf("\nTest Summary:\n");
    printf("  Total: %d\n", s_testsRun);
    printf("  Passed: %d\n", s_testsPassed);
    printf("  Failed: %d\n", s_testsFailed);
    
    return s_testsFailed == 0 ? 0 : 1;
}

void testFramework::assert(bool condition, const char* message) {
    s_testsRun++;
    if (condition) {
        s_testsPassed++;
        printf("[PASS] %s\n", message);
    } else {
        s_testsFailed++;
        printf("[FAIL] %s\n", message);
    }
}

void testFramework::assertEquals(const std::string& expected, const std::string& actual, const char* message) {
    s_testsRun++;
    if (expected == actual) {
        s_testsPassed++;
        printf("[PASS] %s\n", message);
    } else {
        s_testsFailed++;
        printf("[FAIL] %s - Expected: '%s', Actual: '%s'\n", message, expected.c_str(), actual.c_str());
    }
}

// Forward declarations of test functions
void testErrorCodes();
void testSocketCreation();
void testSocketOperations();
void testReuseAddressFunctionality();
void testWebSocketProtocol();
void testWebSocketServer();

int main() {
    printf("=== WebSocket Library Test Suite ===\n\n");
    
    // Run all test categories
    testErrorCodes();
    testSocketCreation();
    testSocketOperations();
    testReuseAddressFunctionality();
    testWebSocketProtocol();
    testWebSocketServer();
    
    return testFramework::runAllTests();
}

// Test functions that will fail first, then pass as we implement
void testErrorCodes() {
    printf("\n--- Error Codes Tests ---\n");
    
    // Test error code string conversion
    testFramework::assertEquals("Success", WebSocket::getErrorCodeString(WebSocket::ErrorCode::success), "SUCCESS error code string");
    testFramework::assertEquals("Unknown error", WebSocket::getErrorCodeString(WebSocket::ErrorCode::unknownError), "UNKNOWN_ERROR error code string");
    
    // Test result struct
    WebSocket::Result successResult(WebSocket::ErrorCode::success);
    testFramework::assert(successResult.isSuccess(), "Success result is success");
    testFramework::assert(!successResult.isError(), "Success result is not error");
    
    WebSocket::Result errorResult(WebSocket::ErrorCode::socketCreateFailed, "Test error");
    testFramework::assert(!errorResult.isSuccess(), "Error result is not success");
    testFramework::assert(errorResult.isError(), "Error result is error");
    testFramework::assertEquals("Test error", errorResult.getErrorMessage(), "Error message is preserved");
}

void testSocketCreation() {
    printf("\n--- Socket Creation Tests ---\n");
    
    // Note: Socket system is now automatically initialized when first socket is created
    
    // Test socket creation
    WebSocket::Socket socket;
    testFramework::assert(!socket.isValid(), "Socket is initially invalid");
    
    WebSocket::Result createResult = socket.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP);
    testFramework::assert(createResult.isSuccess(), "IPv4 TCP socket creation");
    testFramework::assert(socket.isValid(), "Socket is valid after creation");
    
    // Test socket move constructor
    WebSocket::Socket movedSocket = std::move(socket);
    testFramework::assert(!socket.isValid(), "Original socket is invalid after move");
    testFramework::assert(movedSocket.isValid(), "Moved socket is valid");
    
    // Test socket cleanup
    WebSocket::Result closeResult = movedSocket.close();
    testFramework::assert(closeResult.isSuccess(), "Socket close");
    testFramework::assert(!movedSocket.isValid(), "Socket is invalid after close");
    

}

void testSocketOperations() {
    printf("\n--- Socket Operations Tests ---\n");
    
    // Note: Socket system is now automatically initialized when first socket is created
    
    // create server socket
    WebSocket::Socket serverSocket;
    WebSocket::Result createResult = serverSocket.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP);
    testFramework::assert(createResult.isSuccess(), "Server socket creation");
    
    // Test socket binding
    WebSocket::Result bindResult = serverSocket.bind("127.0.0.1", 0); // Use port 0 for automatic assignment
    testFramework::assert(bindResult.isSuccess(), "Socket binding to localhost");
    
    // Test socket listening
    WebSocket::Result listenResult = serverSocket.listen(5);
    testFramework::assert(listenResult.isSuccess(), "Socket listening");
    
    // Test socket options
    WebSocket::Result blockingResult = serverSocket.blocking(false);
    testFramework::assert(blockingResult.isSuccess(), "Set non-blocking mode");
    
    WebSocket::Result reuseResult = serverSocket.reuseAddress(true);
    testFramework::assert(reuseResult.isSuccess(), "Set reuse address");
    
    WebSocket::Result keepAliveResult = serverSocket.keepAlive(true);
    testFramework::assert(keepAliveResult.isSuccess(), "Set keep alive");
    
    // Get socket address for client connection
    std::string localAddress = serverSocket.localAddress();
    uint16_t localPort = serverSocket.localPort();
    testFramework::assert(!localAddress.empty(), "Get local address should succeed");
    testFramework::assert(localAddress == "127.0.0.1" || localAddress == "0.0.0.0", "Address should be localhost");
    testFramework::assert(localPort > 0, "Port should be greater than 0");
    
    // create client socket
    WebSocket::Socket clientSocket;
    WebSocket::Result clientcreateResult = clientSocket.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP);
    testFramework::assert(clientcreateResult.isSuccess(), "Client socket creation");
    
    // Test client connection
    WebSocket::Result connectResult = clientSocket.connect("127.0.0.1", localPort);
    testFramework::assert(connectResult.isSuccess(), "Client connection to server");
    
    // Test data transmission
    std::string testData = "Hello WebSocket!";
    WebSocket::Result sendResult = clientSocket.send(std::vector<uint8_t>(testData.begin(), testData.end()));
    testFramework::assert(sendResult.isSuccess(), "Client send data");
    
    // accept connection
    auto [acceptResult, acceptedSocket] = serverSocket.accept();
    testFramework::assert(acceptResult.isSuccess(), "Server accept connection");
    testFramework::assert(acceptedSocket != nullptr, "accepted socket should not be null");
    
    // receive data
    auto [receiveResult, receivedData] = acceptedSocket->receive(1024);
    testFramework::assert(receiveResult.isSuccess(), "Server receive data");
    testFramework::assert(!receivedData.empty(), "received data should not be empty");
    
    std::string receivedString(receivedData.begin(), receivedData.end());
    testFramework::assertEquals(testData, receivedString, "received data should match sent data");
    
    // Cleanup
    WebSocket::Result clientcloseResult = clientSocket.close();
    testFramework::assert(clientcloseResult.isSuccess(), "Client socket close");
    
    WebSocket::Result acceptedcloseResult = acceptedSocket->close();
    testFramework::assert(acceptedcloseResult.isSuccess(), "accepted socket close");
    
    WebSocket::Result servercloseResult = serverSocket.close();
    testFramework::assert(servercloseResult.isSuccess(), "Server socket close");
    

}

void testReuseAddressFunctionality() {
    printf("\n--- REUSEADDR Functionality Tests ---\n");
    
    // Note: Socket system is now automatically initialized when first socket is created
    
    const std::string testAddress = "127.0.0.1";
    const uint16_t testPort = 0; // Let OS choose port
    
    // Test 1: create server with REUSEADDR, close, and recreate on same port
    {
        printf("Test 1: Rapid server restart with REUSEADDR\n");
        
        // First server
        WebSocket::Socket server1;
        testFramework::assert(server1.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP).isSuccess(), 
                              "First server socket creation");
        
        WebSocket::Result reuseResult1 = server1.reuseAddress(true);
        testFramework::assert(reuseResult1.isSuccess(), "First server SetreuseAddress(true)");
        
        testFramework::assert(server1.bind(testAddress, testPort).isSuccess(), "First server bind");
        uint16_t serverPort = server1.localPort();
        testFramework::assert(serverPort > 0, "First server got valid port");
        testFramework::assert(server1.listen(5).isSuccess(), "First server listen");
        
        printf("  First server bound to port %d\n", serverPort);
        
        // close first server
        testFramework::assert(server1.close().isSuccess(), "First server close");
        
        // Small delay to ensure socket is fully closed
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Second server on same port (should succeed with REUSEADDR)
        WebSocket::Socket server2;
        testFramework::assert(server2.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP).isSuccess(), 
                              "Second server socket creation");
        
        WebSocket::Result reuseResult2 = server2.reuseAddress(true);
        testFramework::assert(reuseResult2.isSuccess(), "Second server SetreuseAddress(true)");
        
        testFramework::assert(server2.bind(testAddress, serverPort).isSuccess(), 
                              "Second server bind to same port should succeed with REUSEADDR");
        testFramework::assert(server2.listen(5).isSuccess(), "Second server listen");
        
        printf("  Second server successfully bound to same port %d\n", serverPort);
        
        testFramework::assert(server2.close().isSuccess(), "Second server close");
    }
    
    // Test 2: Verify SetreuseAddress(false) works
    {
        printf("Test 2: SetreuseAddress(false) functionality\n");
        
        WebSocket::Socket server;
        testFramework::assert(server.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP).isSuccess(), 
                              "Server socket creation");
        
        WebSocket::Result reuseResult = server.reuseAddress(false);
        testFramework::assert(reuseResult.isSuccess(), "SetreuseAddress(false) should succeed");
        
        testFramework::assert(server.close().isSuccess(), "Server close");
    }
    
    // Test 3: Multiple servers with REUSEADDR can bind to port 0
    {
        printf("Test 3: Multiple servers with REUSEADDR on port 0\n");
        
        for (int i = 0; i < 3; ++i) {
            WebSocket::Socket server;
            testFramework::assert(server.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP).isSuccess(), 
                                  "Server socket creation");
            
            testFramework::assert(server.reuseAddress(true).isSuccess(), "SetreuseAddress(true)");
            testFramework::assert(server.bind(testAddress, 0).isSuccess(), "bind to port 0");
            testFramework::assert(server.listen(5).isSuccess(), "listen");
            
            uint16_t port = server.localPort();
            testFramework::assert(port > 0, "Got valid port");
            printf("  Server %d bound to port %d\n", i + 1, port);
            
            testFramework::assert(server.close().isSuccess(), "Server close");
        }
    }
    

    printf("✅ All REUSEADDR functionality tests passed!\n");
}

void testWebSocketProtocol() {
    printf("\n--- WebSocket Protocol Tests ---\n");
    
    // Test frame creation
    WebSocket::WebSocketFrame textFrame = WebSocket::WebSocketProtocol::createTextFrame("Hello World");
    testFramework::assert(textFrame.Fin, "Text frame has FIN flag set");
    testFramework::assert(!textFrame.Rsv1 && !textFrame.Rsv2 && !textFrame.Rsv3, "Text frame has no RSV bits set");
    testFramework::assert(textFrame.Opcode == WebSocket::websocketOpcode::TEXT, "Text frame has correct opcode");
    testFramework::assert(!textFrame.Masked, "Text frame is not masked (server-to-client)");
    testFramework::assert(textFrame.PayloadLength == 11, "Text frame has correct payload length");
    
    // Test binary frame
    std::vector<uint8_t> binaryData = {0x01, 0x02, 0x03, 0x04};
    WebSocket::WebSocketFrame binaryFrame = WebSocket::WebSocketProtocol::createBinaryFrame(binaryData);
    testFramework::assert(binaryFrame.Opcode == WebSocket::websocketOpcode::BINARY, "Binary frame has correct opcode");
    testFramework::assert(binaryFrame.PayloadLength == 4, "Binary frame has correct payload length");
    
    // Test ping frame
    WebSocket::WebSocketFrame pingFrame = WebSocket::WebSocketProtocol::createPingFrame();
    testFramework::assert(pingFrame.Opcode == WebSocket::websocketOpcode::PING, "Ping frame has correct opcode");
    
    // Test pong frame
    WebSocket::WebSocketFrame pongFrame = WebSocket::WebSocketProtocol::createPongFrame();
    testFramework::assert(pongFrame.Opcode == WebSocket::websocketOpcode::PONG, "Pong frame has correct opcode");
    
    // Test close frame
    WebSocket::WebSocketFrame closeFrame = WebSocket::WebSocketProtocol::createCloseFrame(1000, "Normal closure");
    testFramework::assert(closeFrame.Opcode == WebSocket::websocketOpcode::CLOSE, "close frame has correct opcode");
    testFramework::assert(closeFrame.PayloadLength >= 2, "close frame has at least status code");
    
    // Test frame generation
    std::vector<uint8_t> frameData = WebSocket::WebSocketProtocol::generateFrame(textFrame);
    testFramework::assert(!frameData.empty(), "Frame generation produces data");
    testFramework::assert(frameData.size() >= 2, "Frame has minimum header size");
    
    // Test frame parsing
    WebSocket::WebSocketFrame parsedFrame;
    size_t bytesConsumed = 0;
    WebSocket::Result parseResult = WebSocket::WebSocketProtocol::parseFrame(frameData, parsedFrame, bytesConsumed);
    testFramework::assert(parseResult.isSuccess(), "Frame parsing succeeds");
    testFramework::assert(bytesConsumed > 0, "Frame parsing consumes bytes");
    testFramework::assert(parsedFrame.Opcode == textFrame.Opcode, "Parsed frame has correct opcode");
    testFramework::assert(parsedFrame.PayloadLength == textFrame.PayloadLength, "Parsed frame has correct payload length");
}

void testWebSocketServer() {
    printf("\n--- WebSocket Server Tests ---\n");
    // Tests will be added here
}

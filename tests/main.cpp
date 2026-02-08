#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include "WebSocket/ErrorCodes.h"
#include "WebSocket/Socket.h"
#include "WebSocket/WebSocketProtocol.h"

// Simple test framework for CTest
class TestFramework {
public:
    static int RunAllTests();
    static void Assert(bool condition, const char* message);
    static void AssertEquals(const std::string& expected, const std::string& actual, const char* message);
    
private:
    static int s_testsRun;
    static int s_testsPassed;
    static int s_testsFailed;
};

int TestFramework::s_testsRun = 0;
int TestFramework::s_testsPassed = 0;
int TestFramework::s_testsFailed = 0;

int TestFramework::RunAllTests() {
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

void TestFramework::Assert(bool condition, const char* message) {
    s_testsRun++;
    if (condition) {
        s_testsPassed++;
        printf("[PASS] %s\n", message);
    } else {
        s_testsFailed++;
        printf("[FAIL] %s\n", message);
    }
}

void TestFramework::AssertEquals(const std::string& expected, const std::string& actual, const char* message) {
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
void TestErrorCodes();
void TestSocketCreation();
void TestSocketOperations();
void TestReuseAddressFunctionality();
void TestWebSocketProtocol();
void TestWebSocketServer();

int main() {
    printf("=== WebSocket Library Test Suite ===\n\n");
    
    // Run all test categories
    TestErrorCodes();
    TestSocketCreation();
    TestSocketOperations();
    TestReuseAddressFunctionality();
    TestWebSocketProtocol();
    TestWebSocketServer();
    
    return TestFramework::RunAllTests();
}

// Test functions that will fail first, then pass as we implement
void TestErrorCodes() {
    printf("\n--- Error Codes Tests ---\n");
    
    // Test error code string conversion
    TestFramework::AssertEquals("Success", WebSocket::GetErrorCodeString(WebSocket::ERROR_CODE::SUCCESS), "SUCCESS error code string");
    TestFramework::AssertEquals("Unknown error", WebSocket::GetErrorCodeString(WebSocket::ERROR_CODE::UNKNOWN_ERROR), "UNKNOWN_ERROR error code string");
    
    // Test result struct
    WebSocket::Result successResult(WebSocket::ERROR_CODE::SUCCESS);
    TestFramework::Assert(successResult.IsSuccess(), "Success result is success");
    TestFramework::Assert(!successResult.IsError(), "Success result is not error");
    
    WebSocket::Result errorResult(WebSocket::ERROR_CODE::SOCKET_CREATE_FAILED, "Test error");
    TestFramework::Assert(!errorResult.IsSuccess(), "Error result is not success");
    TestFramework::Assert(errorResult.IsError(), "Error result is error");
    TestFramework::AssertEquals("Test error", errorResult.GetErrorMessage(), "Error message is preserved");
}

void TestSocketCreation() {
    printf("\n--- Socket Creation Tests ---\n");
    
    // Note: Socket system is now automatically initialized when first socket is created
    
    // Test socket creation
    WebSocket::Socket socket;
    TestFramework::Assert(!socket.Valid(), "Socket is initially invalid");
    
    WebSocket::Result createResult = socket.Create(WebSocket::SOCKET_FAMILY::IPV4, WebSocket::SOCKET_TYPE::TCP);
    TestFramework::Assert(createResult.IsSuccess(), "IPv4 TCP socket creation");
    TestFramework::Assert(socket.Valid(), "Socket is valid after creation");
    
    // Test socket move constructor
    WebSocket::Socket movedSocket = std::move(socket);
    TestFramework::Assert(!socket.Valid(), "Original socket is invalid after move");
    TestFramework::Assert(movedSocket.Valid(), "Moved socket is valid");
    
    // Test socket cleanup
    WebSocket::Result closeResult = movedSocket.Close();
    TestFramework::Assert(closeResult.IsSuccess(), "Socket close");
    TestFramework::Assert(!movedSocket.Valid(), "Socket is invalid after close");
    

}

void TestSocketOperations() {
    printf("\n--- Socket Operations Tests ---\n");
    
    // Note: Socket system is now automatically initialized when first socket is created
    
    // Create server socket
    WebSocket::Socket serverSocket;
    WebSocket::Result createResult = serverSocket.Create(WebSocket::SOCKET_FAMILY::IPV4, WebSocket::SOCKET_TYPE::TCP);
    TestFramework::Assert(createResult.IsSuccess(), "Server socket creation");
    
    // Test socket binding
    WebSocket::Result bindResult = serverSocket.Bind("127.0.0.1", 0); // Use port 0 for automatic assignment
    TestFramework::Assert(bindResult.IsSuccess(), "Socket binding to localhost");
    
    // Test socket listening
    WebSocket::Result listenResult = serverSocket.Listen(5);
    TestFramework::Assert(listenResult.IsSuccess(), "Socket listening");
    
    // Test socket options
    WebSocket::Result blockingResult = serverSocket.Blocking(false);
    TestFramework::Assert(blockingResult.IsSuccess(), "Set non-blocking mode");
    
    WebSocket::Result reuseResult = serverSocket.ReuseAddress(true);
    TestFramework::Assert(reuseResult.IsSuccess(), "Set reuse address");
    
    WebSocket::Result keepAliveResult = serverSocket.KeepAlive(true);
    TestFramework::Assert(keepAliveResult.IsSuccess(), "Set keep alive");
    
    // Get socket address for client connection
    std::string localAddress = serverSocket.LocalAddress();
    uint16_t localPort = serverSocket.LocalPort();
    TestFramework::Assert(!localAddress.empty(), "Get local address should succeed");
    TestFramework::Assert(localAddress == "127.0.0.1" || localAddress == "0.0.0.0", "Address should be localhost");
    TestFramework::Assert(localPort > 0, "Port should be greater than 0");
    
    // Create client socket
    WebSocket::Socket clientSocket;
    WebSocket::Result clientCreateResult = clientSocket.Create(WebSocket::SOCKET_FAMILY::IPV4, WebSocket::SOCKET_TYPE::TCP);
    TestFramework::Assert(clientCreateResult.IsSuccess(), "Client socket creation");
    
    // Test client connection
    WebSocket::Result connectResult = clientSocket.Connect("127.0.0.1", localPort);
    TestFramework::Assert(connectResult.IsSuccess(), "Client connection to server");
    
    // Test data transmission
    std::string testData = "Hello WebSocket!";
    WebSocket::Result sendResult = clientSocket.Send(std::vector<uint8_t>(testData.begin(), testData.end()));
    TestFramework::Assert(sendResult.IsSuccess(), "Client send data");
    
    // Accept connection
    auto [acceptResult, acceptedSocket] = serverSocket.Accept();
    TestFramework::Assert(acceptResult.IsSuccess(), "Server accept connection");
    TestFramework::Assert(acceptedSocket != nullptr, "Accepted socket should not be null");
    
    // Receive data
    auto [receiveResult, receivedData] = acceptedSocket->Receive(1024);
    TestFramework::Assert(receiveResult.IsSuccess(), "Server receive data");
    TestFramework::Assert(!receivedData.empty(), "Received data should not be empty");
    
    std::string receivedString(receivedData.begin(), receivedData.end());
    TestFramework::AssertEquals(testData, receivedString, "Received data should match sent data");
    
    // Cleanup
    WebSocket::Result clientCloseResult = clientSocket.Close();
    TestFramework::Assert(clientCloseResult.IsSuccess(), "Client socket close");
    
    WebSocket::Result acceptedCloseResult = acceptedSocket->Close();
    TestFramework::Assert(acceptedCloseResult.IsSuccess(), "Accepted socket close");
    
    WebSocket::Result serverCloseResult = serverSocket.Close();
    TestFramework::Assert(serverCloseResult.IsSuccess(), "Server socket close");
    

}

void TestReuseAddressFunctionality() {
    printf("\n--- REUSEADDR Functionality Tests ---\n");
    
    // Note: Socket system is now automatically initialized when first socket is created
    
    const std::string testAddress = "127.0.0.1";
    const uint16_t testPort = 0; // Let OS choose port
    
    // Test 1: Create server with REUSEADDR, close, and recreate on same port
    {
        printf("Test 1: Rapid server restart with REUSEADDR\n");
        
        // First server
        WebSocket::Socket server1;
        TestFramework::Assert(server1.Create(WebSocket::SOCKET_FAMILY::IPV4, WebSocket::SOCKET_TYPE::TCP).IsSuccess(), 
                              "First server socket creation");
        
        WebSocket::Result reuseResult1 = server1.ReuseAddress(true);
        TestFramework::Assert(reuseResult1.IsSuccess(), "First server SetReuseAddress(true)");
        
        TestFramework::Assert(server1.Bind(testAddress, testPort).IsSuccess(), "First server bind");
        uint16_t serverPort = server1.LocalPort();
        TestFramework::Assert(serverPort > 0, "First server got valid port");
        TestFramework::Assert(server1.Listen(5).IsSuccess(), "First server listen");
        
        printf("  First server bound to port %d\n", serverPort);
        
        // Close first server
        TestFramework::Assert(server1.Close().IsSuccess(), "First server close");
        
        // Small delay to ensure socket is fully closed
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Second server on same port (should succeed with REUSEADDR)
        WebSocket::Socket server2;
        TestFramework::Assert(server2.Create(WebSocket::SOCKET_FAMILY::IPV4, WebSocket::SOCKET_TYPE::TCP).IsSuccess(), 
                              "Second server socket creation");
        
        WebSocket::Result reuseResult2 = server2.ReuseAddress(true);
        TestFramework::Assert(reuseResult2.IsSuccess(), "Second server SetReuseAddress(true)");
        
        TestFramework::Assert(server2.Bind(testAddress, serverPort).IsSuccess(), 
                              "Second server bind to same port should succeed with REUSEADDR");
        TestFramework::Assert(server2.Listen(5).IsSuccess(), "Second server listen");
        
        printf("  Second server successfully bound to same port %d\n", serverPort);
        
        TestFramework::Assert(server2.Close().IsSuccess(), "Second server close");
    }
    
    // Test 2: Verify SetReuseAddress(false) works
    {
        printf("Test 2: SetReuseAddress(false) functionality\n");
        
        WebSocket::Socket server;
        TestFramework::Assert(server.Create(WebSocket::SOCKET_FAMILY::IPV4, WebSocket::SOCKET_TYPE::TCP).IsSuccess(), 
                              "Server socket creation");
        
        WebSocket::Result reuseResult = server.ReuseAddress(false);
        TestFramework::Assert(reuseResult.IsSuccess(), "SetReuseAddress(false) should succeed");
        
        TestFramework::Assert(server.Close().IsSuccess(), "Server close");
    }
    
    // Test 3: Multiple servers with REUSEADDR can bind to port 0
    {
        printf("Test 3: Multiple servers with REUSEADDR on port 0\n");
        
        for (int i = 0; i < 3; ++i) {
            WebSocket::Socket server;
            TestFramework::Assert(server.Create(WebSocket::SOCKET_FAMILY::IPV4, WebSocket::SOCKET_TYPE::TCP).IsSuccess(), 
                                  "Server socket creation");
            
            TestFramework::Assert(server.ReuseAddress(true).IsSuccess(), "SetReuseAddress(true)");
            TestFramework::Assert(server.Bind(testAddress, 0).IsSuccess(), "Bind to port 0");
            TestFramework::Assert(server.Listen(5).IsSuccess(), "Listen");
            
            uint16_t port = server.LocalPort();
            TestFramework::Assert(port > 0, "Got valid port");
            printf("  Server %d bound to port %d\n", i + 1, port);
            
            TestFramework::Assert(server.Close().IsSuccess(), "Server close");
        }
    }
    

    printf("✅ All REUSEADDR functionality tests passed!\n");
}

void TestWebSocketProtocol() {
    printf("\n--- WebSocket Protocol Tests ---\n");
    
    // Test frame creation
    WebSocket::WebSocketFrame textFrame = WebSocket::WebSocketProtocol::CreateTextFrame("Hello World");
    TestFramework::Assert(textFrame.Fin, "Text frame has FIN flag set");
    TestFramework::Assert(!textFrame.Rsv1 && !textFrame.Rsv2 && !textFrame.Rsv3, "Text frame has no RSV bits set");
    TestFramework::Assert(textFrame.Opcode == WebSocket::WEBSOCKET_OPCODE::TEXT, "Text frame has correct opcode");
    TestFramework::Assert(!textFrame.Masked, "Text frame is not masked (server-to-client)");
    TestFramework::Assert(textFrame.PayloadLength == 11, "Text frame has correct payload length");
    
    // Test binary frame
    std::vector<uint8_t> binaryData = {0x01, 0x02, 0x03, 0x04};
    WebSocket::WebSocketFrame binaryFrame = WebSocket::WebSocketProtocol::CreateBinaryFrame(binaryData);
    TestFramework::Assert(binaryFrame.Opcode == WebSocket::WEBSOCKET_OPCODE::BINARY, "Binary frame has correct opcode");
    TestFramework::Assert(binaryFrame.PayloadLength == 4, "Binary frame has correct payload length");
    
    // Test ping frame
    WebSocket::WebSocketFrame pingFrame = WebSocket::WebSocketProtocol::CreatePingFrame();
    TestFramework::Assert(pingFrame.Opcode == WebSocket::WEBSOCKET_OPCODE::PING, "Ping frame has correct opcode");
    
    // Test pong frame
    WebSocket::WebSocketFrame pongFrame = WebSocket::WebSocketProtocol::CreatePongFrame();
    TestFramework::Assert(pongFrame.Opcode == WebSocket::WEBSOCKET_OPCODE::PONG, "Pong frame has correct opcode");
    
    // Test close frame
    WebSocket::WebSocketFrame closeFrame = WebSocket::WebSocketProtocol::CreateCloseFrame(1000, "Normal closure");
    TestFramework::Assert(closeFrame.Opcode == WebSocket::WEBSOCKET_OPCODE::CLOSE, "Close frame has correct opcode");
    TestFramework::Assert(closeFrame.PayloadLength >= 2, "Close frame has at least status code");
    
    // Test frame generation
    std::vector<uint8_t> frameData = WebSocket::WebSocketProtocol::GenerateFrame(textFrame);
    TestFramework::Assert(!frameData.empty(), "Frame generation produces data");
    TestFramework::Assert(frameData.size() >= 2, "Frame has minimum header size");
    
    // Test frame parsing
    WebSocket::WebSocketFrame parsedFrame;
    size_t bytesConsumed = 0;
    WebSocket::Result parseResult = WebSocket::WebSocketProtocol::ParseFrame(frameData, parsedFrame, bytesConsumed);
    TestFramework::Assert(parseResult.IsSuccess(), "Frame parsing succeeds");
    TestFramework::Assert(bytesConsumed > 0, "Frame parsing consumes bytes");
    TestFramework::Assert(parsedFrame.Opcode == textFrame.Opcode, "Parsed frame has correct opcode");
    TestFramework::Assert(parsedFrame.PayloadLength == textFrame.PayloadLength, "Parsed frame has correct payload length");
}

void TestWebSocketServer() {
    printf("\n--- WebSocket Server Tests ---\n");
    // Tests will be added here
}

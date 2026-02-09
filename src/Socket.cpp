#include "WebSocket/Socket.h"
#include "WebSocket/AddrInfo.h"
#include "WebSocket/ErrorCodes.h"
#include "WebSocket/OS.h"
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <algorithm>

// Helper function for network byte order (avoid redeclaration)
uint16_t my_htons(uint16_t hostshort) {
    return (hostshort << 8) | ((hostshort >> 8) & 0xFF);
}

namespace nob {

    // Static member definitions
    std::atomic<int> Socket::s_socketCount{ 0 };
    std::mutex Socket::s_initMutex;

	Socket::Socket()
		: m_isBlocking(true)
		, m_isListening(false) {
		puts("Socket created");
	}

Socket::~Socket() {
		stopEventLoop();

		// Clean up async I/O resources
		if (m_asyncEnabled.load()) {
			m_asyncEnabled.store(false);
		}

		// Only close if we haven't been closed already
		// Close() will handle the reference counting
		if (isValid()) {
			close();
		}
		// Note: If socket was already closed, reference counting was already handled in Close()
	}

	Socket::Socket(Socket&& other) noexcept
		: SocketBase(std::move(other))
		, m_isBlocking(other.m_isBlocking)
		, m_isListening(other.m_isListening) {
		// Move constructor - SocketBase handles socket transfer
		other.m_isBlocking = true;
		other.m_isListening = false;
	}

	Socket& Socket::operator=(Socket&& other) noexcept {
		if (this != &other) {
			close();
			SocketBase::operator=(std::move(other));
			m_isBlocking = other.m_isBlocking;
			m_isListening = other.m_isListening;
			other.m_isBlocking = true;
			other.m_isListening = false;
		}
		return *this;
	}

	Result Socket::InitializeSocketSystem() {
#ifdef _WIN32
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0) {
			return Result(ErrorCode::socketCreateFailed, "WSAStartup failed: " + std::to_string(result));
		}
		return Result();
#else
		return Result();
#endif
	}

	void Socket::CleanupSocketSystem() {
#ifdef _WIN32
		WSACleanup();
#endif
	}

	Result Socket::create(socketFamily family, socketType type) {
		if (isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket already created");
		}

		// Automatic socket system initialization - thread-safe with reference counting
		{
			std::lock_guard<std::mutex> lock(s_initMutex);
			int previousCount = s_socketCount.fetch_add(1);
			if (previousCount == 0) {
				// First socket - initialize the socket system
				Result initResult = InitializeSocketSystem();
				if (initResult.isError()) {
					s_socketCount.fetch_sub(1);
					return initResult;
				}
			}
		}

		int af = (family == socketFamily::IPV4) ? AF_INET_VALUE : AF_INET_VALUE;
		int sockType = (type == socketType::TCP) ? SOCK_STREAM_VALUE : SOCK_DGRAM;
		int protocol = (type == socketType::TCP) ? IPPROTO_TCP : IPPROTO_UDP;

		return createNativeSocket(af, sockType, protocol);
	}

	Result Socket::bind(const std::string& address, uint16_t port) {
		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		// Determine if this is IPv6 or IPv4
		bool isIPv6 = isIPv6Address(address);
		
		if (isIPv6 || address == "::") {
			// IPv6 binding - not supported by current SocketAddress (IPv4 only)
			// For now, return error for IPv6
			return Result(ErrorCode::invalidParameter, "IPv6 not supported by current SocketAddress");
		} else {
			// IPv4 binding (default)
			SocketBase::SocketAddress addr;
			addr.setFamily(AF_INET_VALUE);
			addr.setPort(my_htons(port));

			if (address.empty() || address == "0.0.0.0") {
				addr.setAddress(INADDR_ANY);
			}
			else {
				uint32_t addressValue;
				if (inet_pton(AF_INET_VALUE, address.c_str(), &addressValue) != 1) {
					return Result(ErrorCode::invalidParameter, "Invalid IPv4 address: " + address);
				}
				addr.setAddress(addressValue);
			}

			return bindNativeSocket(addr);
		}
	}

	Result Socket::listen(int backlog) {
		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		Result result = listenNativeSocket(backlog);
		if (result.isSuccess()) {
			m_isListening = true;
		}
		return result;
	}

	AcceptResult Socket::accept() {
		if (!isValid()) {
			return { Result(ErrorCode::invalidParameter, "Socket not created"), nullptr };
		}

		// Use type-safe SocketAddress for IPv4 only
		SocketBase::SocketAddress clientAddr;
		
		NativeSocketTypes::SocketType clientSocket = acceptNativeSocket(clientAddr);
		if (clientSocket == INVALID_SOCKET_NATIVE) {
			return { Result(ErrorCode::socketAcceptFailed, getLastSystemErrorCode()), nullptr };
		}

		auto newSocket = createFromNative(clientSocket);
		return { Result(), std::move(newSocket) };
	}

	Result Socket::connect(const std::string& address, uint16_t port) {
		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		SocketBase::SocketAddress addr;
		uint32_t addressValue = INADDR_ANY;
		if (inet_pton(AF_INET_VALUE, address.c_str(), &addressValue) != 1) {
			return Result(ErrorCode::invalidParameter, "Invalid IPv4 address: " + address);
		}
		
		addr.setFamily(AF_INET_VALUE);
		addr.setAddress(addressValue);
		addr.setPort(my_htons(port));

		return connectNativeSocket(addr);
	}

	Result Socket::shutdown() {
		if (!isValid()) {
			return Result();
		}

#ifdef _WIN32
		return shutdownNativeSocket(SD_BOTH);
#else
		return shutdownNativeSocket(SHUT_RDWR);
#endif
	}

	Result Socket::close() {
		if (!isValid()) {
			return Result();
		}

		// Graceful shutdown first
		shutdown();

		// Use the inherited closeNativeSocket method
		Result result = closeNativeSocket();

		// Automatic socket system cleanup - thread-safe with reference counting
		{
			std::lock_guard<std::mutex> lock(s_initMutex);
			int previousCount = s_socketCount.fetch_sub(1);
			if (previousCount == 1) {
				// Last socket - cleanup the socket system
				CleanupSocketSystem();
			}
		}

		if (result.isError()) {
			updateLastError();
			return Result(ErrorCode::unknownError, getLastSystemErrorCode());
		}

		return Result();
	}

	SendResult Socket::sendRaw(const void* data, size_t length) {
		if (!isValid()) {
			return { Result(ErrorCode::invalidParameter, "Socket not created"), 0 };
		}

		if (!data || length == 0) {
			return { Result(ErrorCode::invalidParameter, "Invalid data parameters"), 0 };
		}

		size_t bytesSent = 0;
		Result result = sendNativeSocket(data, length, &bytesSent);

		return { result, bytesSent };
	}

	ReceiveResult Socket::receiveRaw(void* buffer, size_t bufferSize) {
		if (!isValid()) {
			return { Result(ErrorCode::invalidParameter, "Socket not created"), {} };
		}

		if (!buffer || bufferSize == 0) {
			return { Result(ErrorCode::invalidParameter, "Invalid buffer parameters"), {} };
		}

		size_t bytesReceived = 0;
		Result result = receiveNativeSocket(buffer, bufferSize, &bytesReceived);

		if (result.isError()) {
			return { result, {} };
		}

		// Create vector from received data
		std::vector<uint8_t> data((uint8_t*)buffer, (uint8_t*)buffer + bytesReceived);
		return { Result(), data };
	}

	Result Socket::send(const std::vector<uint8_t>& data) {
		auto [result, bytesSent] = sendRaw(data.data(), data.size());
		return result;
	}

	ReceiveResult Socket::receive(size_t maxLength) {
		std::vector<uint8_t> buffer;
		buffer.reserve(maxLength);
		buffer.resize(maxLength);
		auto [result, received] = receiveRaw(buffer.data(), maxLength);

		if (result.isError()) {
			return { result, {} };
		}

		return { result, received };
	}

	ReceiveResult Socket::receive(size_t maxLength, int timeoutMs) {
		if (!isValid()) {
			return {Result(ErrorCode::invalidParameter, "Socket is not valid"), {}};
		}

		auto nativeSocket = getNativeSocket();
		// Use select() to implement timeout
		fd_set readfds;
		FD_ZERO(&readfds);
#ifdef _WIN32
		FD_SET(reinterpret_cast<SOCKET>(nativeSocket), &readfds);
#else
		FD_SET(static_cast<int>(nativeSocket), &readfds);
#endif

		struct timeval timeout;
		timeout.tv_sec = timeoutMs / 1000;
		timeout.tv_usec = (timeoutMs % 1000) * 1000;

		int selectResult = select(
#ifdef _WIN32
			reinterpret_cast<SOCKET>(nativeSocket) + 1,
#else
			static_cast<int>(nativeSocket) + 1,
#endif
			&readfds, nullptr, nullptr, &timeout);

		if (selectResult < 0) {
			updateLastError();
			return {Result(ErrorCode::socketReceiveFailed, getLastSystemErrorCode()), {}};
		}

		if (selectResult == 0) {
			// Timeout, no data available
			return {Result(), {}};
		}

		// Socket is ready for reading, proceed with normal receive
		std::vector<uint8_t> buffer;
		buffer.reserve(maxLength);
		buffer.resize(maxLength);
		auto [result, received] = receiveRaw(buffer.data(), maxLength);

		if (result.isError()) {
			return { result, {} };
		}

		return { result, received };
	}

	Result Socket::blocking(bool blocking) {
		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		Result result = setBlocking(blocking);
		if (result.isSuccess()) {
			m_isBlocking = blocking;
		}
		return result;
	}

	Result Socket::reuseAddress(bool reuse) {
		int value = reuse ? 1 : 0;
		return setSocketOption(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
	}

	Result Socket::keepAlive(bool keepAlive) {
		int value = keepAlive ? 1 : 0;
		return setSocketOption(SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
	}

	Result Socket::sendBufferSize(size_t size) {
		int value = (int)size;
		return setSocketOption(SOL_SOCKET, SO_SNDBUF, &value, sizeof(value));
	}

	Result Socket::receiveBufferSize(size_t size) {
		int value = (int)size;
		return setSocketOption(SOL_SOCKET, SO_RCVBUF, &value, sizeof(value));
	}

	bool Socket::isValid() const {
		return SocketBase::isValid();
	}

	bool Socket::isBlocking() const {
		return m_isBlocking;
	}

	std::string Socket::localAddress() const {
		if (!isValid()) {
			return "";
		}

		// Use type-safe SocketAddress
		SocketBase::SocketAddress addr;
		Result result = getSocketNameNative(addr);
		if (result.isError()) {
			return "";
		}

		// Convert SocketAddress to string
		char buffer[INET_ADDRSTRLEN];
		struct sockaddr_in sockAddr;
		sockAddr.sin_family = addr.family;
		sockAddr.sin_addr.s_addr = addr.getAddress();
		sockAddr.sin_port = addr.getPort();
		memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

		inet_ntop(AF_INET, &sockAddr.sin_addr, buffer, INET_ADDRSTRLEN);
		return std::string(buffer);
	}

	uint16_t Socket::localPort() const {
		if (!isValid()) {
			return 0;
		}

		// Use type-safe SocketAddress
		SocketBase::SocketAddress addr;
		Result result = getSocketNameNative(addr);
		if (result.isError()) {
			return 0;
		}

		return ntohs(addr.getPort());
	}

	std::string Socket::remoteAddress() const {
		if (!isValid()) {
			return "";
		}

		// Use type-safe SocketAddress
		SocketBase::SocketAddress addr;
		Result result = getPeerNameNative(addr);
		if (result.isError()) {
			return "";
		}

		// Convert SocketAddress to string
		char buffer[INET_ADDRSTRLEN];
		struct sockaddr_in sockAddr;
		sockAddr.sin_family = addr.family;
		sockAddr.sin_addr.s_addr = addr.getAddress();
		sockAddr.sin_port = addr.getPort();
		memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

		inet_ntop(AF_INET, &sockAddr.sin_addr, buffer, INET_ADDRSTRLEN);
		return std::string(buffer);
	}

	uint16_t Socket::remotePort() const {
		if (!isValid()) {
			return 0;
		}

		// Use type-safe SocketAddress
		SocketBase::SocketAddress addr;
		Result result = getPeerNameNative(addr);
		if (result.isError()) {
			return 0;
		}

		return ntohs(addr.getPort());
	}

	std::string Socket::getAddressString(const struct sockaddr* addr) {
		if (!addr) {
			return "";
		}

		char buffer[INET_ADDRSTRLEN];
		const struct sockaddr_in* addr4 = (const struct sockaddr_in*)addr;

		if (inet_ntop(AF_INET, &addr4->sin_addr, buffer, sizeof(buffer)) != nullptr) {
			return std::string(buffer);
		}

		return "";
	}

	bool Socket::isIPAddress(const std::string& address) {
		return isIPv4Address(address) || isIPv6Address(address);
	}

	bool Socket::isIPv4Address(const std::string& address) {
		if (address.empty()) return false;
		
		struct sockaddr_in addr;
		return inet_pton(AF_INET, address.c_str(), &addr.sin_addr) == 1;
	}

	bool Socket::isIPv6Address(const std::string& address) {
		if (address.empty()) return false;
		
		struct sockaddr_in6 addr;
		return inet_pton(AF_INET6, address.c_str(), &addr.sin6_addr) == 1;
	}

	bool Socket::isPortAvailable(uint16_t port, const std::string& address) {
		// Note: address parameter is currently unused but kept for API compatibility
		(void)address; // Suppress unused parameter warning
		
		// Initialize socket system if needed (since this is a static method)
		Result initResult = InitializeSocketSystem();
		if (!initResult.isSuccess()) {
			return false;
		}
		
		// Create a temporary SocketBase instance to test port availability
		SocketBase testSocket;
		
		// Create test socket using abstract
		Result result = testSocket.createNativeSocket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
		if (result.isError()) {
			CleanupSocketSystem();
			return false;
		}
		
		// Create address for binding
		SocketBase::SocketAddress addr;
		addr.setFamily(AF_INET_VALUE);
		addr.setAddress(INADDR_ANY);
		addr.setPort(my_htons(port));
		
		// Try to bind to the port
		result = testSocket.bindNativeSocket(addr);
		
		// Clean up
		testSocket.closeNativeSocket();
		CleanupSocketSystem();
		
		return result.isSuccess();
	}

	std::vector<std::string> Socket::getLocalIPAddresses() {
		std::vector<std::string> ipAddresses;
		
#ifdef _WIN32
		// Windows implementation using modern Winsock APIs
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			return ipAddresses; // Return empty list on error
		}
		
		// Get host information using getaddrinfo with RAII protection
		char hostname[256] = {0};
		if (gethostname(hostname, sizeof(hostname)) != 0) {
			WSACleanup();
			return ipAddresses;
		}
		
		::addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC; // Both IPv4 and IPv6
		hints.ai_socktype = SOCK_STREAM;
		
		// Use RAII wrapper for automatic cleanup
		ADDRINFOA* result = nullptr;
		int getaddrResult = GetAddrInfoA(hostname, nullptr, &hints, &result);
		if (getaddrResult != 0) {
			WSACleanup();
			return ipAddresses; // Return empty list on error
		}
		
		AddrInfo addrInfo((struct addrinfo*)result, true);
		
		// Extract all IP addresses using range-based for loop
		for (const auto& addr : addrInfo) {
			const ::addrinfo* global_addr = (const ::addrinfo*)&addr;
			if (global_addr->ai_family == AF_INET) {
				struct sockaddr_in* sockaddr = (struct sockaddr_in*)global_addr->ai_addr;
				char buffer[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &(sockaddr->sin_addr), buffer, INET_ADDRSTRLEN) != nullptr) {
					std::string ip = buffer;
					
					// Add if not duplicate and not empty
					if (!ip.empty() && std::find(ipAddresses.begin(), ipAddresses.end(), ip) == ipAddresses.end()) {
						ipAddresses.push_back(ip);
					}
				}
			}
			else if (global_addr->ai_family == AF_INET6) {
				struct sockaddr_in6* sockaddr = (struct sockaddr_in6*)global_addr->ai_addr;
				char buffer[INET6_ADDRSTRLEN];
				if (inet_ntop(AF_INET6, &(sockaddr->sin6_addr), buffer, INET6_ADDRSTRLEN) != nullptr) {
					std::string ip = buffer;
					
					// Add if not duplicate
					if (!ip.empty() && std::find(ipAddresses.begin(), ipAddresses.end(), ip) == ipAddresses.end()) {
						ipAddresses.push_back(ip);
					}
				}
			}
		}
		
		// No need for manual freeaddrinfo - RAII handles it automatically
		WSACleanup();
		
#else
		// POSIX implementation using getifaddrs
		struct ifaddrs* ifaddrsPtr = nullptr;
		
		if (getifaddrs(&ifaddrsPtr) == -1) {
			return ipAddresses; // Return empty list on error
		}
		
		for (struct ifaddrs* ifa = ifaddrsPtr; ifa != nullptr; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == nullptr) continue;
			
			// Only interested in IPv4 and IPv6 addresses
			if (ifa->ifa_addr->sa_family == AF_INET) {
				struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
				char buffer[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &(addr->sin_addr), buffer, INET_ADDRSTRLEN) != nullptr) {
					std::string ip = buffer;
					
					// Add if not duplicate and not loopback (unless we want it)
					if (!ip.empty() && std::find(ipAddresses.begin(), ipAddresses.end(), ip) == ipAddresses.end()) {
						ipAddresses.push_back(ip);
					}
				}
			}
			else if (ifa->ifa_addr->sa_family == AF_INET6) {
				struct sockaddr_in6* addr = (struct sockaddr_in6*)ifa->ifa_addr;
				char buffer[INET6_ADDRSTRLEN];
				if (inet_ntop(AF_INET6, &(addr->sin6_addr), buffer, INET6_ADDRSTRLEN) != nullptr) {
					std::string ip = buffer;
					
					// Add if not duplicate
					if (!ip.empty() && std::find(ipAddresses.begin(), ipAddresses.end(), ip) == ipAddresses.end()) {
						ipAddresses.push_back(ip);
					}
				}
			}
		}
		
		freeifaddrs(ifaddrsPtr);
#endif
		
		// Sort the IP addresses for consistent output
		std::sort(ipAddresses.begin(), ipAddresses.end());
		
		return ipAddresses;
	}

	Result Socket::setSocketOption(int level, int option, const void* value, size_t length) {
		return setSocketOptionNative(level, option, value, length);
	}

	Result Socket::getSocketOption(int level, int option, void* value, size_t* length) const {
		return getSocketOptionNative(level, option, value, length);
	}

	void Socket::updateLastError() {
		int systemErrorCode = getLastSystemErrorCode();
		// Only log actual errors, not expected non-blocking behavior
		if (systemErrorCode != 0) {
			std::string systemError = getSystemErrorMessage(systemErrorCode);
			if (!systemError.empty() && systemError.find("would block") == std::string::npos
				&& systemError.find("non-blocking") == std::string::npos) {
				printf("Socket error: %s\n", systemError.c_str());
			}
		}
	}

	std::pair<std::string, uint16_t> Socket::getSocketAddress(const struct sockaddr* addr) {
		if (!addr) {
			return { "", 0 };
		}

		const struct sockaddr_in* addr4 = (const struct sockaddr_in*)addr;
		char buffer[INET_ADDRSTRLEN];

		if (inet_ntop(AF_INET, &addr4->sin_addr, buffer, sizeof(buffer)) != nullptr) {
			return { std::string(buffer), ntohs(addr4->sin_port) };
		}

		return { "", 0 };
	}

	GetAddressResult Socket::getSocketAddress() const {
		if (!isValid()) {
			return { Result(ErrorCode::invalidParameter, "Socket not created"), {"", 0} };
		}

		// Use type-safe SocketAddress
		SocketBase::SocketAddress addr;
		Result result = getSocketNameNative(addr);
		if (result.isError()) {
			return { result, {"", 0} };
		}

		// Convert SocketAddress to string representation
		char buffer[INET_ADDRSTRLEN];
		struct sockaddr_in sockAddr;
		sockAddr.sin_family = addr.family;
		sockAddr.sin_addr.s_addr = addr.getAddress();
		sockAddr.sin_port = addr.getPort();
		memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

		inet_ntop(AF_INET, &sockAddr.sin_addr, buffer, INET_ADDRSTRLEN);
		return { result, {buffer, ntohs(addr.getPort())} };
	}

	// Event Loop Implementation
	Result Socket::startEventLoop() {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);

		if (m_eventLoopRunning.load()) {
			return Result(ErrorCode::unknownError, "Event loop is already running");
		}

		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket is not valid");
		}

		m_eventLoopRunning.store(true);
		m_eventLoopThread = std::make_unique<std::thread>(&Socket::eventLoopFunction, this);

		return Result();
	}

	Result Socket::stopEventLoop() {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);

		if (!m_eventLoopRunning.load()) {
			return Result(); // Already stopped
		}

		m_eventLoopRunning.store(false);

		if (m_eventLoopThread && m_eventLoopThread->joinable()) {
			m_eventLoopThread->join();
		}

		m_eventLoopThread.reset();
		return Result();
	}

	bool Socket::isEventLoopRunning() const {
		return m_eventLoopRunning.load();
	}

	void Socket::acceptCallback(AcceptCallbackFn callback) {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);
		m_acceptCallback = std::move(callback);
	}

	void Socket::receiveCallback(ReceiveCallbackFn callback) {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);
		m_receiveCallback = std::move(callback);
	}

	void Socket::errorCallback(ErrorCallbackFn callback) {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);
		m_errorCallback = std::move(callback);
	}

	Result Socket::sendAsync(const std::vector<uint8_t>& data) {
		if (!m_asyncEnabled.load()) {
			return Result(ErrorCode::invalidParameter, "Async I/O not enabled");
		}

		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		if (data.empty()) {
			return Result(ErrorCode::invalidParameter, "Empty data");
		}

		size_t bytesSent = 0;
		Result result = sendAsync(data.data(), data.size(), &bytesSent);
		return result;
	}

	Result Socket::receiveAsync(size_t maxLength) {
		if (!m_asyncEnabled.load()) {
			return Result(ErrorCode::invalidParameter, "Async I/O not enabled");
		}

		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		if (maxLength == 0) {
			return Result(ErrorCode::invalidParameter, "Invalid max length");
		}

		std::vector<uint8_t> tempBuffer;
		tempBuffer.resize(maxLength);
		size_t bytesReceived = 0;
		Result result = receiveAsync(tempBuffer.data(), maxLength, &bytesReceived);

		if (result.isSuccess() && bytesReceived > 0) {
			tempBuffer.resize(bytesReceived);
			if (m_receiveCallback) {
				m_receiveCallback(tempBuffer);
			}
		}

		return result;
	}

	bool Socket::isAsyncEnabled() const {
		return m_asyncEnabled.load();
	}

	void Socket::eventLoopFunction() {
		while (m_eventLoopRunning.load()) {
			Result result = processSocketEvents();
			if (!result.isSuccess()) {
				if (m_errorCallback) {
					m_errorCallback(result);
				}
				// Don't break on error, continue trying
			}

			// Small sleep to prevent CPU spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	Result Socket::processSocketEvents() {
		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket is not valid");
		}

		// Use selectNativeSocket to check for socket events
		bool canRead = false, canWrite = false;
		Result result = selectNativeSocket(10, &canRead, &canWrite);

		if (result.isError()) {
			return result;
		}

		// Check if socket is ready for reading
		if (canRead) {
			if (m_isListening) {
				// This is a listening socket, only handle accept events
				handleAcceptEvent();
			}
			else {
				// This is a connected socket, handle receive events
				handleReceiveEvent();
			}
		}

		return Result();
	}

	void Socket::handleAcceptEvent() {
		// Try to accept a connection
		SocketBase::SocketAddress clientAddr;
		
		NativeSocketTypes::SocketType clientSocket = acceptNativeSocket(clientAddr);

		if (clientSocket != 
#ifdef _WIN32
			reinterpret_cast<NativeSocketTypes::SocketType>(INVALID_SOCKET)
#else
			static_cast<NativeSocketTypes::SocketType>(-1)
#endif
		) {
			// Successfully accepted a connection
			auto newSocket = createFromNative(clientSocket);
			if (newSocket) {
				// Set the new socket to non-blocking for event loop
				newSocket->blocking(false);

				if (m_acceptCallback) {
					m_acceptCallback(std::move(newSocket));
				}
			}
		}
		// If accept fails, it's normal (no pending connections)
	}

	void Socket::handleReceiveEvent() {
		// This is a connected socket, try to receive data
		char buffer[4096];
		size_t bytesReceived = 0;
		Result result = receiveNativeSocket(buffer, sizeof(buffer), &bytesReceived);

		if (result.isSuccess() && bytesReceived > 0) {
			// Data received
			std::vector<uint8_t> data(buffer, buffer + bytesReceived);
			if (m_receiveCallback) {
				m_receiveCallback(data);
			}
		}
		else if (result.isSuccess() && bytesReceived == 0) {
			// Connection closed
			if (m_errorCallback) {
				m_errorCallback(Result(ErrorCode::websocketConnectionClosed, "Connection closed by peer"));
			}
		}
		else {
			// Error occurred - check if it's a non-blocking error
			int systemErrorCode = getLastSystemErrorCode();
#ifdef _WIN32
			if (systemErrorCode != WSAEWOULDBLOCK) {
#else
			if (systemErrorCode != EAGAIN && systemErrorCode != EWOULDBLOCK) {
#endif
				if (m_errorCallback) {
					m_errorCallback(Result(ErrorCode::socketReceiveFailed, systemErrorCode));
				}
			}
		}
	}

	Socket::Socket(NativeSocketTypes::SocketType nativeSocket) : SocketBase() {
	// Set the native socket handle
	setNativeSocket(nativeSocket);
	
	// Initialize state
	m_isBlocking = true;
	m_isListening = false;
	
	// This socket was created outside of Create(), so we need to increment the counter
	std::lock_guard<std::mutex> lock(s_initMutex);
	int previousCount = s_socketCount.fetch_add(1);
	if (previousCount == 0) {
		// This shouldn't happen if the system was properly initialized,
		// but we handle it just in case
		Result initResult = InitializeSocketSystem();
		if (initResult.isError()) {
			s_socketCount.fetch_sub(1);
			// Note: We can't throw from constructor, so we'll leave it in an invalid state
		}
	}
}

std::unique_ptr<Socket> Socket::createFromNative(NativeSocketTypes::SocketType nativeSocket) {
		auto socket = std::unique_ptr<Socket>(new Socket(nativeSocket));

		// This socket was created outside of Create(), so we need to increment the counter
		std::lock_guard<std::mutex> lock(s_initMutex);
		int previousCount = s_socketCount.fetch_add(1);
		if (previousCount == 0) {
			// This shouldn't happen if the system was properly initialized,
			// but we handle it just in case
			Result initResult = InitializeSocketSystem();
			if (initResult.isError()) {
				s_socketCount.fetch_sub(1);
				return nullptr;
			}
		}

		return socket;
	}

// Additional async I/O method implementations
	Result Socket::sendAsync(const void* data, size_t length, size_t* bytesSent) {
		if (!m_asyncEnabled.load()) {
			return Result(ErrorCode::invalidParameter, "Async I/O not enabled");
		}

		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		if (!data || length == 0 || !bytesSent) {
			return Result(ErrorCode::invalidParameter, "Invalid parameters");
		}

		return SocketBase::sendAsync(data, length, bytesSent);
	}

	Result Socket::receiveAsync(void* buffer, size_t bufferSize, size_t* bytesReceived) {
		if (!m_asyncEnabled.load()) {
			return Result(ErrorCode::invalidParameter, "Async I/O not enabled");
		}

		if (!isValid()) {
			return Result(ErrorCode::invalidParameter, "Socket not created");
		}

		if (!buffer || bufferSize == 0 || !bytesReceived) {
			return Result(ErrorCode::invalidParameter, "Invalid parameters");
		}

		return SocketBase::receiveAsync(buffer, bufferSize, bytesReceived);
	}

} // namespace nob

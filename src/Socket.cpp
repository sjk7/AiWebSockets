#include "WebSocket/Socket.h"
#include "WebSocket/AddrInfoGuard.h"
#include "WebSocket/ErrorCodes.h"
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#endif

namespace WebSocket {

	// Static member definitions
	std::atomic<int> Socket::s_socketCount{ 0 };
	std::mutex Socket::s_initMutex;

	Socket::Socket()
		: m_socket(INVALID_SOCKET_NATIVE)
		, m_isBlocking(true)
		, m_isListening(false) {
		puts("Socket created");
	}

	Socket::Socket(SOCKET_TYPE_NATIVE nativeSocket)
		: m_socket(nativeSocket)
		, m_isBlocking(true)
		, m_isListening(false) {
	}

	Socket::~Socket() {
		StopEventLoop();

		// Clean up async I/O resources
		if (m_asyncEnabled.load()) {
#ifdef _WIN32
			if (m_completionPort != nullptr) {
				CloseHandle(m_completionPort);
				m_completionPort = nullptr;
			}
#else
			if (m_epollFd != -1) {
				close(m_epollFd);
				m_epollFd = -1;
			}
#endif
			m_asyncEnabled.store(false);
		}

		// Only close if we haven't been closed already
		// Close() will handle the reference counting
		if (Valid()) {
			close();
		}
		// Note: If socket was already closed, reference counting was already handled in Close()
	}

	Socket::Socket(Socket&& other) noexcept
		: m_socket(other.m_socket)
		, m_isBlocking(other.m_isBlocking)
		, m_isListening(other.m_isListening) {
		// Move constructor - no change to socket count since we're just transferring ownership
		other.m_socket = INVALID_SOCKET_NATIVE;
		other.m_isBlocking = true;
		other.m_isListening = false;
	}

	Socket& Socket::operator=(Socket&& other) noexcept {
		if (this != &other) {
			close();
			m_socket = other.m_socket;
			m_isBlocking = other.m_isBlocking;
			m_isListening = other.m_isListening;
			other.m_socket = INVALID_SOCKET_NATIVE;
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
			return Result(ERROR_CODE::SOCKET_CREATE_FAILED, "WSAStartup failed: " + std::to_string(result));
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

	Result Socket::create(SOCKET_FAMILY family, SOCKET_TYPE type) {
		if (Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket already created");
		}

		// Automatic socket system initialization - thread-safe with reference counting
		{
			std::lock_guard<std::mutex> lock(s_initMutex);
			int previousCount = s_socketCount.fetch_add(1);
			if (previousCount == 0) {
				// First socket - initialize the socket system
				Result initResult = InitializeSocketSystem();
				if (initResult.IsError()) {
					s_socketCount.fetch_sub(1);
					return initResult;
				}
			}
		}

		int af = (family == SOCKET_FAMILY::IPV4) ? AF_INET : AF_INET6;
		int sockType = (type == SOCKET_TYPE::TCP) ? SOCK_STREAM : SOCK_DGRAM;
		int protocol = (type == SOCKET_TYPE::TCP) ? IPPROTO_TCP : IPPROTO_UDP;

		m_socket = socket(af, sockType, protocol);
		if (m_socket == INVALID_SOCKET_NATIVE) {
			return Result(ERROR_CODE::SOCKET_CREATE_FAILED, GetLastSystemErrorCode());
		}

		return Result();
	}

	Result Socket::bind(const std::string& address, uint16_t port) {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		// Determine if this is IPv6 or IPv4
		bool isIPv6 = IsIPv6Address(address);
		
		if (isIPv6 || address == "::") {
			// IPv6 binding
			struct sockaddr_in6 addr6;
			std::memset(&addr6, 0, sizeof(addr6));
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(port);
			
			if (address.empty() || address == "::") {
				addr6.sin6_addr = in6addr_any;
			} else {
				if (inet_pton(AF_INET6, address.c_str(), &addr6.sin6_addr) != 1) {
					return Result(ERROR_CODE::INVALID_PARAMETER, "Invalid IPv6 address: " + address);
				}
			}
			
			if (::bind(m_socket, (struct sockaddr*)&addr6, sizeof(addr6)) != 0) {
				UpdateLastError();
				int systemErrorCode = GetLastSystemErrorCode();
				std::string systemError = GetSystemErrorMessage(systemErrorCode);
				if (systemError.find("address already in use") != std::string::npos || 
					systemError.find("Only one usage of each socket address") != std::string::npos) {
					std::string portError = "Port " + std::to_string(port) + " is already in use. " + systemError;
					return Result(ERROR_CODE::SOCKET_BIND_FAILED, portError);
				}
				return Result(ERROR_CODE::SOCKET_BIND_FAILED, systemErrorCode);
			}
		} else {
			// IPv4 binding (default)
			struct sockaddr_in addr;
			std::memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);

			if (address.empty() || address == "0.0.0.0") {
				addr.sin_addr.s_addr = INADDR_ANY;
			}
			else {
				if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
					return Result(ERROR_CODE::INVALID_PARAMETER, "Invalid IPv4 address: " + address);
				}
			}

			if (::bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
				UpdateLastError();
				int systemErrorCode = GetLastSystemErrorCode();
				// Provide more specific error message for port in use
				std::string systemError = GetSystemErrorMessage(systemErrorCode);
				if (systemError.find("address already in use") != std::string::npos || 
					systemError.find("Only one usage of each socket address") != std::string::npos) {
					// Custom message for port conflicts - this is cached immediately
					std::string portError = "Port " + std::to_string(port) + " is already in use. " + systemError;
					return Result(ERROR_CODE::SOCKET_BIND_FAILED, portError);
				}
				return Result(ERROR_CODE::SOCKET_BIND_FAILED, systemErrorCode);
			}
		}

		return Result();
	}

	Result Socket::listen(int backlog) {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		if (::listen(m_socket, backlog) != 0) {
			UpdateLastError();
			return Result(ERROR_CODE::SOCKET_LISTEN_FAILED, GetLastSystemErrorCode());
		}

		m_isListening = true;
		return Result();
	}

	AcceptResult Socket::accept() {
		if (!Valid()) {
			return { Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created"), nullptr };
		}

		// Use a large enough buffer for both IPv4 and IPv6 addresses
		struct sockaddr_storage clientAddr;
		socklen_t clientAddrLen = sizeof(clientAddr);
		std::memset(&clientAddr, 0, sizeof(clientAddr));

		SOCKET_TYPE_NATIVE clientSocket = ::accept(m_socket, (struct sockaddr*)&clientAddr, &clientAddrLen);
		if (clientSocket == INVALID_SOCKET_NATIVE) {
			UpdateLastError();
			const auto ret = Result(ERROR_CODE::SOCKET_ACCEPT_FAILED, GetLastSystemErrorCode());
			return { ret, nullptr };
		}

		auto newSocket = CreateFromNative(clientSocket);
		return { Result(), std::move(newSocket) };
	}

	Result Socket::connect(const std::string& address, uint16_t port) {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		struct sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Invalid IPv4 address: " + address);
		}

		if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
			UpdateLastError();
			return Result(ERROR_CODE::SOCKET_CONNECT_FAILED, GetLastSystemErrorCode());
		}

		return Result();
	}

	Result Socket::shutdown() {
		if (!Valid()) {
			return Result();
		}

#ifdef _WIN32
		int result = ::shutdown(m_socket, SD_BOTH);
#else
		int result = ::shutdown(m_socket, SHUT_RDWR);
#endif

		if (result == SOCKET_ERROR) {
			UpdateLastError();
			return Result(ERROR_CODE::UNKNOWN_ERROR, GetLastSystemErrorCode());
		}

		return Result();
	}

	Result Socket::close() {
		if (!Valid()) {
			return Result();
		}

		// Graceful shutdown first
		shutdown();

#ifdef _WIN32
		int result = closesocket(m_socket);
#else
		int result = close(m_socket);
#endif

		m_socket = INVALID_SOCKET_NATIVE;

		// Automatic socket system cleanup - thread-safe with reference counting
		{
			std::lock_guard<std::mutex> lock(s_initMutex);
			int previousCount = s_socketCount.fetch_sub(1);
			if (previousCount == 1) {
				// Last socket - cleanup the socket system
				CleanupSocketSystem();
			}
		}

		if (result != 0) {
			UpdateLastError();
			return Result(ERROR_CODE::UNKNOWN_ERROR, GetLastSystemErrorCode());
		}

		return Result();
	}

	SendResult Socket::sendRaw(const void* data, size_t length) {
		if (!Valid()) {
			return { Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created"), 0 };
		}

		if (!data || length == 0) {
			return { Result(ERROR_CODE::INVALID_PARAMETER, "Invalid data parameters"), 0 };
		}

		size_t totalSent = 0;
		const char* buffer = static_cast<const char*>(data);

		while (totalSent < length) {
#ifdef _WIN32
			int result = ::send(m_socket, buffer + totalSent, (int)(length - totalSent), 0);
#else
			ssize_t result = ::send(m_socket, buffer + totalSent, length - totalSent, 0);
#endif

			if (result < 0) {
				UpdateLastError();
				return { Result(ERROR_CODE::SOCKET_SEND_FAILED, GetLastSystemErrorCode()), totalSent };
			}

			// Handle partial sends (result == 0 means connection closed)
			if (result == 0) {
				printf("DEBUG: Send returned 0 - connection closed\n");
				break; // Connection closed
			}

			totalSent += result;
			// printf("DEBUG: Send progress: %zu bytes this call, %zu total\n", (size_t)result, totalSent);
		}

		// printf("DEBUG: Send completed successfully - %zu bytes sent\n", totalSent);
		return { Result(), totalSent };
	}

	std::pair<Result, std::vector<uint8_t>> Socket::receiveRaw(void* buffer, size_t bufferSize) {
		if (!Valid()) {
			return { Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created"), {} };
		}

		if (!buffer || bufferSize == 0) {
			return { Result(ERROR_CODE::INVALID_PARAMETER, "Invalid buffer parameters"), {} };
		}

#ifdef _WIN32
		int result = recv(m_socket, (char*)buffer, (int)bufferSize, 0);
#else
		ssize_t result = recv(m_socket, buffer, bufferSize, 0);
#endif

		if (result < 0) {
			UpdateLastError();
			return { Result(ERROR_CODE::SOCKET_RECEIVE_FAILED, GetLastSystemErrorCode()), {} };
		}

		// result == 0 means connection closed gracefully
		if (result == 0) {
			return { Result(), {} };
		}

		// Create vector from received data
		std::vector<uint8_t> data((uint8_t*)buffer, (uint8_t*)buffer + result);
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

		if (result.IsError()) {
			return { result, {} };
		}

		return { result, received };
	}

	ReceiveResult Socket::receive(size_t maxLength, int timeoutMs) {
		if (!Valid()) {
			return {Result(ERROR_CODE::INVALID_PARAMETER, "Socket is not valid"), {}};
		}

		// Use select() to implement timeout
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(m_socket, &readfds);

		struct timeval timeout;
		timeout.tv_sec = timeoutMs / 1000;
		timeout.tv_usec = (timeoutMs % 1000) * 1000;

		int selectResult = select(
#ifdef _WIN32
			m_socket + 1,
#else
			m_socket + 1,
#endif
			&readfds, nullptr, nullptr, &timeout);

		if (selectResult < 0) {
			UpdateLastError();
			return {Result(ERROR_CODE::SOCKET_RECEIVE_FAILED, GetLastSystemErrorCode()), {}};
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

		if (result.IsError()) {
			return { result, {} };
		}

		return { result, received };
	}

	Result Socket::Blocking(bool blocking) {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

#ifdef _WIN32
		u_long mode = blocking ? 0 : 1;
		int result = ioctlsocket(m_socket, FIONBIO, &mode);
		if (result != 0) {
			UpdateLastError();
			return Result(ERROR_CODE::SOCKET_SET_OPTION_FAILED, GetLastSystemErrorCode());
		}
#else
		int flags = fcntl(m_socket, F_GETFL, 0);
		if (flags == -1) {
			UpdateLastError();
			return Result(ERROR_CODE::SOCKET_SET_OPTION_FAILED, GetLastSystemErrorCode());
		}

		int result = fcntl(m_socket, F_SETFL, blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
		if (result == -1) {
			UpdateLastError();
			return Result(ERROR_CODE::SOCKET_SET_OPTION_FAILED, GetLastSystemErrorCode());
		}
#endif

		m_isBlocking = blocking;
		return Result();
	}

	Result Socket::ReuseAddress(bool reuse) {
		int value = reuse ? 1 : 0;
		return SetSocketOption(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
	}

	Result Socket::KeepAlive(bool keepAlive) {
		int value = keepAlive ? 1 : 0;
		return SetSocketOption(SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
	}

	Result Socket::SendBufferSize(size_t size) {
		int value = (int)size;
		return SetSocketOption(SOL_SOCKET, SO_SNDBUF, &value, sizeof(value));
	}

	Result Socket::ReceiveBufferSize(size_t size) {
		int value = (int)size;
		return SetSocketOption(SOL_SOCKET, SO_RCVBUF, &value, sizeof(value));
	}

	bool Socket::Valid() const {
		return m_socket != INVALID_SOCKET_NATIVE;
	}

	bool Socket::Blocking() const {
		return m_isBlocking;
	}

	std::string Socket::LocalAddress() const {
		if (!Valid()) {
			return "";
		}

		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);

		if (getsockname(m_socket, (struct sockaddr*)&addr, &addrLen) != 0) {
			return "";
		}

		return GetAddressString((struct sockaddr*)&addr);
	}

	uint16_t Socket::LocalPort() const {
		if (!Valid()) {
			return 0;
		}

		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);

		if (getsockname(m_socket, (struct sockaddr*)&addr, &addrLen) != 0) {
			return 0;
		}

		return ntohs(addr.sin_port);
	}

	std::string Socket::RemoteAddress() const {
		if (!Valid()) {
			return "";
		}

		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);

		if (getpeername(m_socket, (struct sockaddr*)&addr, &addrLen) != 0) {
			return "";
		}

		return GetAddressString((struct sockaddr*)&addr);
	}

	uint16_t Socket::RemotePort() const {
		if (!Valid()) {
			return 0;
		}

		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);

		if (getpeername(m_socket, (struct sockaddr*)&addr, &addrLen) != 0) {
			return 0;
		}

		return ntohs(addr.sin_port);
	}

	std::string Socket::GetAddressString(const struct sockaddr* addr) {
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

	bool Socket::IsIPAddress(const std::string& address) {
		return IsIPv4Address(address) || IsIPv6Address(address);
	}

	bool Socket::IsIPv4Address(const std::string& address) {
		if (address.empty()) return false;
		
		struct sockaddr_in addr;
		return inet_pton(AF_INET, address.c_str(), &addr.sin_addr) == 1;
	}

	bool Socket::IsIPv6Address(const std::string& address) {
		if (address.empty()) return false;
		
		struct sockaddr_in6 addr;
		return inet_pton(AF_INET6, address.c_str(), &addr.sin6_addr) == 1;
	}

	bool Socket::IsPortAvailable(uint16_t port, const std::string& address) {
		// Initialize socket system if needed (since this is a static method)
		Result initResult = InitializeSocketSystem();
		if (!initResult.IsSuccess()) {
			printf("DEBUG: IsPortAvailable - Failed to initialize socket system: %s\n", initResult.GetErrorMessage().c_str());
			return false;
		}

		// Determine if this is IPv6 or IPv4
		bool isIPv6 = IsIPv6Address(address);
		
		SOCKET_TYPE_NATIVE testSocket;
		void* addrPtr;
		socklen_t addrLen;
		
		if (isIPv6 || address == "::") {
			// IPv6 test
			testSocket = socket(AF_INET6, SOCK_STREAM, 0);
			if (testSocket == INVALID_SOCKET_NATIVE) {
				printf("DEBUG: IsPortAvailable - Failed to create IPv6 test socket\n");
				CleanupSocketSystem();
				return false;
			}
			
			struct sockaddr_in6 addr6;
			std::memset(&addr6, 0, sizeof(addr6));
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(port);
			
			if (address.empty() || address == "::") {
				addr6.sin6_addr = in6addr_any;
			} else {
				if (inet_pton(AF_INET6, address.c_str(), &addr6.sin6_addr) != 1) {
					printf("DEBUG: IsPortAvailable - Invalid IPv6 address: %s\n", address.c_str());
#ifdef _WIN32
					closesocket(testSocket);
#else
					close(testSocket);
#endif
					CleanupSocketSystem();
					return false;
				}
			}
			
			addrPtr = &addr6;
			addrLen = sizeof(addr6);
		} else {
			// IPv4 test (default)
			testSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (testSocket == INVALID_SOCKET_NATIVE) {
				printf("DEBUG: IsPortAvailable - Failed to create IPv4 test socket\n");
				CleanupSocketSystem();
				return false;
			}
			
			struct sockaddr_in addr;
			std::memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);

			if (address.empty() || address == "0.0.0.0") {
				addr.sin_addr.s_addr = INADDR_ANY;
			}
			else {
				if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
					printf("DEBUG: IsPortAvailable - Invalid IPv4 address: %s\n", address.c_str());
#ifdef _WIN32
					closesocket(testSocket);
#else
					close(testSocket);
#endif
					CleanupSocketSystem();
					return false;
				}
			}
			
			addrPtr = &addr;
			addrLen = sizeof(addr);
		}

		// Try to bind to the port
		int result = ::bind(testSocket, (struct sockaddr*)addrPtr, addrLen);
		printf("DEBUG: IsPortAvailable - bind() result for port %d: %d\n", port, result);

		// Close the test socket
#ifdef _WIN32
		closesocket(testSocket);
#else
		close(testSocket);
#endif

		// Clean up socket system since we initialized it
		CleanupSocketSystem();

		// If bind succeeded, port is available
		bool available = (result == 0);
		printf("DEBUG: IsPortAvailable - Port %d is %s\n", port, available ? "available" : "in use");
		return available;
	}

	std::vector<std::string> Socket::GetLocalIPAddresses() {
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
		
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC; // Both IPv4 and IPv6
		hints.ai_socktype = SOCK_STREAM;
		
		// Use RAII wrapper for automatic cleanup
		AddrInfoGuard addrInfo = GetAddrInfo(hostname, nullptr, &hints);
		if (!addrInfo) {
			WSACleanup();
			return ipAddresses; // Return empty list on error
		}
		
		// Extract all IP addresses using range-based for loop
		for (const auto& addr : addrInfo) {
			if (addr.ai_family == AF_INET) {
				struct sockaddr_in* sockaddr = (struct sockaddr_in*)addr.ai_addr;
				char buffer[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &(sockaddr->sin_addr), buffer, INET_ADDRSTRLEN) != nullptr) {
					std::string ip = buffer;
					
					// Add if not duplicate and not empty
					if (!ip.empty() && std::find(ipAddresses.begin(), ipAddresses.end(), ip) == ipAddresses.end()) {
						ipAddresses.push_back(ip);
					}
				}
			}
			else if (addr.ai_family == AF_INET6) {
				struct sockaddr_in6* sockaddr = (struct sockaddr_in6*)addr.ai_addr;
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

	Result Socket::SetSocketOption(int level, int option, const void* value, size_t length) {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		if (setsockopt(m_socket, level, option, (const char*)value, (int)length) != 0) {
			UpdateLastError();
			return Result(ERROR_CODE::SOCKET_SET_OPTION_FAILED, GetLastSystemErrorCode());
		}

		return Result();
	}

	Result Socket::GetSocketOption(int level, int option, void* value, size_t* length) const {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		socklen_t len = (socklen_t)*length;
		if (getsockopt(m_socket, level, option, (char*)value, &len) != 0) {
			// Note: UpdateLastError is not const, so we handle error differently here
			int systemErrorCode = GetLastSystemErrorCode();
			printf("Socket getsockopt error: %s\n", GetSystemErrorMessage(systemErrorCode).c_str());
			return Result(ERROR_CODE::SOCKET_SET_OPTION_FAILED, systemErrorCode);
		}

		*length = len;
		return Result();
	}

	void Socket::UpdateLastError() {
		int systemErrorCode = GetLastSystemErrorCode();
		// Only log actual errors, not expected non-blocking behavior
		if (systemErrorCode != 0) {
			std::string systemError = GetSystemErrorMessage(systemErrorCode);
			if (!systemError.empty() && systemError.find("would block") == std::string::npos
				&& systemError.find("non-blocking") == std::string::npos) {
				printf("Socket error: %s\n", systemError.c_str());
			}
		}
	}

	std::pair<std::string, uint16_t> Socket::GetSocketAddress(const struct sockaddr* addr) const {
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

	std::pair<Result, std::pair<std::string, uint16_t>> Socket::GetSocketAddress() const {
		if (!Valid()) {
			return { Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created"), {"", 0} };
		}

		struct sockaddr addr;
		socklen_t addrLen = sizeof(addr);

		if (getsockname(m_socket, &addr, &addrLen) != 0) {
			// Use proper Windows error handling
#ifdef _WIN32
			int errorCode = WSAGetLastError();
			char errorBuffer[256];
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, errorCode, 0, errorBuffer, sizeof(errorBuffer), nullptr);
			std::string errorMsg = errorBuffer;
#else
			std::string errorMsg = strerror(errno);
#endif
			return { Result(ERROR_CODE::SOCKET_GETSOCKNAME_FAILED, errorMsg), {"", 0} };
		}

		std::pair<std::string, uint16_t> address = GetSocketAddress(&addr);
		if (address.first.empty()) {
			return { Result(ERROR_CODE::SOCKET_ADDRESS_PARSE_FAILED, "Failed to parse socket address"), {"", 0} };
		}

		return { Result(), address };
	}

	// Event Loop Implementation
	Result Socket::StartEventLoop() {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);

		if (m_eventLoopRunning.load()) {
			return Result(ERROR_CODE::UNKNOWN_ERROR, "Event loop is already running");
		}

		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket is not valid");
		}

		m_eventLoopRunning.store(true);
		m_eventLoopThread = std::make_unique<std::thread>(&Socket::EventLoopFunction, this);

		return Result();
	}

	Result Socket::StopEventLoop() {
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

	bool Socket::EventLoopRunning() const {
		return m_eventLoopRunning.load();
	}

	void Socket::AcceptCallback(AcceptCallbackFn callback) {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);
		m_acceptCallback = std::move(callback);
	}

	void Socket::ReceiveCallback(ReceiveCallbackFn callback) {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);
		m_receiveCallback = std::move(callback);
	}

	void Socket::ErrorCallback(ErrorCallbackFn callback) {
		std::lock_guard<std::mutex> lock(m_eventLoopMutex);
		m_errorCallback = std::move(callback);
	}

	// Async I/O Implementation
	Result Socket::EnableAsyncIO() {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		if (m_asyncEnabled.load()) {
			return Result(); // Already enabled
		}

#ifdef _WIN32
		// Create I/O Completion Port
		m_completionPort = CreateIoCompletionPort((HANDLE)m_socket, nullptr, (ULONG_PTR)this, 0);
		if (m_completionPort == nullptr) {
			return Result(ERROR_CODE::UNKNOWN_ERROR, "Failed to create completion port");
		}

		// Initialize overlapped structures
		memset(&m_sendOverlapped, 0, sizeof(m_sendOverlapped));
		memset(&m_recvOverlapped, 0, sizeof(m_recvOverlapped));

#else
		// Create epoll instance
		m_epollFd = epoll_create1(0);
		if (m_epollFd == -1) {
			return Result(ERROR_CODE::UNKNOWN_ERROR, "Failed to create epoll instance");
		}

		// Add socket to epoll
		struct epoll_event event;
		event.events = EPOLLIN | EPOLLOUT | EPOLLET;
		event.data.fd = m_socket;

		if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_socket, &event) == -1) {
			close(m_epollFd);
			m_epollFd = -1;
			return Result(ERROR_CODE::UNKNOWN_ERROR, "Failed to add socket to epoll");
		}
#endif

		m_asyncEnabled.store(true);
		return Result();
	}

	Result Socket::SendAsync(const std::vector<uint8_t>& data) {
		if (!m_asyncEnabled.load()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Async I/O not enabled");
		}

		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		if (data.empty()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "No data to send");
		}

#ifdef _WIN32
		// Use WSASend for async operation
		WSABUF wsaBuf;
		wsaBuf.buf = (CHAR*)data.data();
		wsaBuf.len = (ULONG)data.size();

		DWORD bytesSent = 0;
		int result = WSASend(m_socket, &wsaBuf, 1, &bytesSent, 0, &m_sendOverlapped, nullptr);

		if (result == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSA_IO_PENDING) {
				return Result(ERROR_CODE::SOCKET_SEND_FAILED, "WSASend failed");
			}
			// Operation is pending - will complete asynchronously
		}

#else
		// Use send with MSG_DONTWAIT for non-blocking async operation
		ssize_t result = send(m_socket, data.data(), data.size(), MSG_DONTWAIT);

		if (result == -1) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				return Result(ERROR_CODE::SOCKET_SEND_FAILED, "Async send failed");
			}
			// Would block - operation will complete asynchronously
		}
#endif

		return Result();
	}

	Result Socket::ReceiveAsync(size_t maxLength) {
		if (!m_asyncEnabled.load()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Async I/O not enabled");
		}

		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket not created");
		}

		if (maxLength == 0) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Invalid max length");
		}

#ifdef _WIN32
		// Use WSARecv for async operation
		std::vector<uint8_t> tempBuffer;
		tempBuffer.resize(maxLength);

		WSABUF wsaBuf;
		wsaBuf.buf = (CHAR*)tempBuffer.data();
		wsaBuf.len = (ULONG)maxLength;

		DWORD bytesReceived = 0;
		DWORD flags = 0;
		int result = WSARecv(m_socket, &wsaBuf, 1, &bytesReceived, &flags, &m_recvOverlapped, nullptr);

		if (result == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSA_IO_PENDING) {
				return Result(ERROR_CODE::SOCKET_RECEIVE_FAILED, "WSARecv failed");
			}
			// Operation is pending - will complete asynchronously
		}

#else
		// Use recv with MSG_DONTWAIT for non-blocking async operation
		std::vector<uint8_t> tempBuffer;
		tempBuffer.resize(maxLength);

		ssize_t result = recv(m_socket, tempBuffer.data(), maxLength, MSG_DONTWAIT);

		if (result == -1) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				return Result(ERROR_CODE::SOCKET_RECEIVE_FAILED, "Async receive failed");
			}
			// Would block - operation will complete asynchronously
		}
#endif

		return Result();
	}

	bool Socket::IsAsyncEnabled() const {
		return m_asyncEnabled.load();
	}

	void Socket::EventLoopFunction() {
		while (m_eventLoopRunning.load()) {
			Result result = ProcessSocketEvents();
			if (!result.IsSuccess()) {
				if (m_errorCallback) {
					m_errorCallback(result);
				}
				// Don't break on error, continue trying
			}

			// Small sleep to prevent CPU spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	Result Socket::ProcessSocketEvents() {
		if (!Valid()) {
			return Result(ERROR_CODE::INVALID_PARAMETER, "Socket is not valid");
		}

		// Use select() to check for socket events
		fd_set readfds;
		FD_ZERO(&readfds);

#ifdef _WIN32
		FD_SET(m_socket, &readfds);
#else
		FD_SET(m_socket, &readfds);
#endif

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms timeout

		int selectResult = select(
#ifdef _WIN32
			m_socket + 1,
#else
			m_socket + 1,
#endif
			& readfds, nullptr, nullptr, &timeout);

		if (selectResult < 0) {
			UpdateLastError();
			return Result(ERROR_CODE::SOCKET_RECEIVE_FAILED, GetLastSystemErrorCode());
		}

		if (selectResult == 0) {
			// Timeout, no events
			return Result();
		}

		// Check if socket is ready for reading
		if (FD_ISSET(m_socket, &readfds)) {
			if (m_isListening) {
				// This is a listening socket, only handle accept events
				HandleAcceptEvent();
			}
			else {
				// This is a connected socket, handle receive events
				HandleReceiveEvent();
			}
		}

		return Result();
	}

	void Socket::HandleAcceptEvent() {
		// Try to accept a connection
		sockaddr_in clientAddr;
#ifdef _WIN32
		int clientAddrLen = sizeof(clientAddr);
#else
		socklen_t clientAddrLen = sizeof(clientAddr);
#endif

		SOCKET_TYPE_NATIVE clientSocket = ::accept(m_socket, (struct sockaddr*)&clientAddr, &clientAddrLen);

		if (clientSocket != INVALID_SOCKET_NATIVE) {
			// Successfully accepted a connection
			auto newSocket = CreateFromNative(clientSocket);
			if (newSocket) {
				// Set the new socket to non-blocking for event loop
				newSocket->Blocking(false);

				if (m_acceptCallback) {
					m_acceptCallback(std::move(newSocket));
				}
			}
		}
		// If accept fails, it's normal (no pending connections)
	}

	void Socket::HandleReceiveEvent() {
		// This is a connected socket, try to receive data
		char buffer[4096];
		int result = recv(m_socket, buffer, sizeof(buffer), 0);

		if (result > 0) {
			// Data received
			std::vector<uint8_t> data(buffer, buffer + result);
			if (m_receiveCallback) {
				m_receiveCallback(data);
			}
		}
		else if (result == 0) {
			// Connection closed
			if (m_errorCallback) {
				m_errorCallback(Result(ERROR_CODE::WEBSOCKET_CONNECTION_CLOSED, "Connection closed by peer"));
			}
		}
		else {
			// Error occurred
			int errorCode = WSAGetLastError();
			if (errorCode != WSAEWOULDBLOCK) {
				UpdateLastError();
				if (m_errorCallback) {
					m_errorCallback(Result(ERROR_CODE::SOCKET_RECEIVE_FAILED, GetLastSystemErrorCode()));
				}
			}
		}
	}

	std::unique_ptr<Socket> Socket::CreateFromNative(SOCKET_TYPE_NATIVE nativeSocket) {
		auto socket = std::unique_ptr<Socket>(new Socket(nativeSocket));

		// This socket was created outside of Create(), so we need to increment the counter
		std::lock_guard<std::mutex> lock(s_initMutex);
		int previousCount = s_socketCount.fetch_add(1);
		if (previousCount == 0) {
			// This shouldn't happen if the system was properly initialized,
			// but we handle it just in case
			Result initResult = InitializeSocketSystem();
			if (initResult.IsError()) {
				s_socketCount.fetch_sub(1);
				return nullptr;
			}
		}

		return socket;
	}

} // namespace WebSocket

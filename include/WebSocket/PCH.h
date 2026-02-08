// Precompiled Header - Ready for future use
// Uncomment CMake PCH configuration when project grows

#pragma once

// C++ Standard Library
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <utility>

// Platform-specific headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <pthread.h>
#endif

// WebSocket project headers (forward declarations preferred)
// #include "WebSocket/Types.h"
// #include "WebSocket/ErrorCodes.h"

// Common macros
#define WEBSOCKET_VERSION_MAJOR 1
#define WEBSOCKET_VERSION_MINOR 0
#define WEBSOCKET_VERSION_PATCH 0

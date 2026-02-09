#include "WebSocket/AddrInfo.h"
#include "WebSocket/OS.h"

namespace nob {

AddrInfo::~AddrInfo() noexcept {
    // Only call freeaddrinfo if we own the pointer AND it hasn't been moved-from
    if (owns_ && addrInfo_ != nullptr) {
        freeaddrinfo(addrInfo_);
    }
}

AddrInfo& AddrInfo::operator=(AddrInfo&& other) noexcept {
    if (this != &other) {
        // Clean up current resource
        if (owns_ && addrInfo_ != nullptr) {
            freeaddrinfo(addrInfo_);
        }

        // Move from other
        addrInfo_ = other.addrInfo_;
        owns_ = other.owns_;
        other.addrInfo_ = nullptr;
        other.owns_ = false;
    }
    return *this;
}

AddrInfo GetAddrInfo(const char* node, const char* service, const struct addrinfo* hints) {
    struct addrinfo* result = nullptr;
    int ret = getaddrinfo(node, service, hints, &result);
    
    if (ret != 0 || result == nullptr) {
        return AddrInfo(nullptr, false); // Return empty guard on error
    }
    
    return AddrInfo(result, true);
}

} // namespace nob

#include "WebSocket/AddrInfo.h"
#include "WebSocket/OS.h"

namespace nob {

AddrInfo::~AddrInfo() noexcept {
    // Only call freeaddrinfo if we own the pointer AND it hasn't been moved-from
    if (owns_ && addrInfo_ != nullptr) {
        ::freeaddrinfo((::addrinfo*)addrInfo_);
    }
}

AddrInfo& AddrInfo::operator=(AddrInfo&& other) noexcept {
    if (this != &other) {
        // Clean up current resource if we own it
        if (owns_ && addrInfo_ != nullptr) {
            ::freeaddrinfo((::addrinfo*)addrInfo_);
        }
        
        // Move from other
        addrInfo_ = other.addrInfo_;
        owns_ = other.owns_;
        
        // Leave other in valid state
        other.addrInfo_ = nullptr;
        other.owns_ = false;
    }
    return *this;
}

AddrInfo GetAddrInfo(const char* node, const char* service, const struct addrinfo* hints) {
    ::addrinfo* result = nullptr;
    int ret = ::getaddrinfo(node, service, (const ::addrinfo*)hints, &result);
    
    if (ret != 0 || result == nullptr) {
        return AddrInfo(nullptr, false); // Return empty guard on error
    }
    
    return AddrInfo((struct addrinfo*)result, true);
}

// Iterator implementations moved from header to maintain firewall
AddrInfo::iterator& AddrInfo::iterator::operator++() noexcept {
    if (current_) {
        // Cast to global addrinfo type to access ai_next
        ::addrinfo* global_addrinfo = (::addrinfo*)current_;
        current_ = (struct addrinfo*)global_addrinfo->ai_next;
    }
    return *this;
}

AddrInfo::iterator AddrInfo::iterator::operator++(int) noexcept {
    iterator tmp = *this;
    ++(*this);
    return tmp;
}

} // namespace nob

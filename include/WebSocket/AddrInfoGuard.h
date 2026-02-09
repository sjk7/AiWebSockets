#pragma once

#include "Types.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

namespace WebSocket {

/**
 * @brief RAII wrapper for addrinfo structures
 * 
 * This class provides automatic memory management for addrinfo structures
 * returned by getaddrinfo(). It follows RAII principles and is move-only
 * to prevent accidental copying which would lead to double-free issues.
 */
class AddrInfoGuard {
private:
    struct addrinfo* addrInfo_;
    bool owns_;

public:
    /**
     * @brief Construct from raw addrinfo pointer
     * @param addrInfo Raw pointer from getaddrinfo()
     * @param owns Whether this guard owns the pointer (default: true)
     */
    explicit AddrInfoGuard(struct addrinfo* addrInfo = nullptr, bool owns = true) noexcept
        : addrInfo_(addrInfo), owns_(owns) {}

    /**
     * @brief Destructor - automatically frees the addrinfo if owned and not moved-from
     */
    ~AddrInfoGuard() noexcept {
        // Only call freeaddrinfo if we own the pointer AND it hasn't been moved-from
        if (owns_ && addrInfo_ != nullptr) {
            freeaddrinfo(addrInfo_);
        }
    }

    // Delete copy constructor and copy assignment
    AddrInfoGuard(const AddrInfoGuard&) = delete;
    AddrInfoGuard& operator=(const AddrInfoGuard&) = delete;

    /**
     * @brief Move constructor
     * @param other Guard to move from
     */
    AddrInfoGuard(AddrInfoGuard&& other) noexcept
        : addrInfo_(other.addrInfo_), owns_(other.owns_) {
        other.addrInfo_ = nullptr;
        other.owns_ = false;
    }

    /**
     * @brief Move assignment operator
     * @param other Guard to move from
     * @return Reference to this
     */
    AddrInfoGuard& operator=(AddrInfoGuard&& other) noexcept {
        if (this != &other) {
            // Clean up current resource
            if (owns_ && addrInfo_ != nullptr) {
                freeaddrinfo(addrInfo_);
            }
            
            // Move from other
            addrInfo_ = other.addrInfo_;
            owns_ = other.owns_;
            
            // Clear other
            other.addrInfo_ = nullptr;
            other.owns_ = false;
        }
        return *this;
    }

    /**
     * @brief Get the raw addrinfo pointer
     * @return Raw pointer (may be nullptr)
     */
    struct addrinfo* get() const noexcept {
        return addrInfo_;
    }

    /**
     * @brief Get the raw addrinfo pointer (operator overload)
     * @return Raw pointer
     */
    struct addrinfo* operator->() const noexcept {
        return addrInfo_;
    }

    /**
     * @brief Dereference operator
     * @return Reference to addrinfo
     */
    struct addrinfo& operator*() const noexcept {
        return *addrInfo_;
    }

    /**
     * @brief Check if the guard holds a valid pointer
     * @return true if pointer is not null
     */
    explicit operator bool() const noexcept {
        return addrInfo_ != nullptr;
    }

    /**
     * @brief Release ownership of the pointer
     * @return Raw pointer (caller takes ownership)
     */
    struct addrinfo* release() noexcept {
        struct addrinfo* result = addrInfo_;
        addrInfo_ = nullptr;
        owns_ = false;
        return result;
    }

    /**
     * @brief Reset with a new pointer
     * @param addrInfo New pointer to manage (or nullptr)
     * @param owns Whether this guard owns the pointer
     */
    void reset(struct addrinfo* addrInfo = nullptr, bool owns = true) noexcept {
        if (owns_ && addrInfo_ != nullptr) {
            freeaddrinfo(addrInfo_);
        }
        addrInfo_ = addrInfo;
        owns_ = owns;
    }

    /**
     * @brief Check if this guard owns the pointer
     * @return true if owns the pointer
     */
    bool owns() const noexcept {
        return owns_;
    }

    /**
     * @brief Check if this guard is in a valid state (not moved-from)
     * @return true if the guard is valid and can be safely used
     */
    bool isValid() const noexcept {
        return owns_ && addrInfo_ != nullptr;
    }

    /**
     * @brief Iterator support for range-based for loops
     */
    struct iterator {
        struct addrinfo* current;
        
        iterator(struct addrinfo* ptr) : current(ptr) {}
        
        struct addrinfo& operator*() const noexcept { return *current; }
        struct addrinfo* operator->() const noexcept { return current; }
        
        iterator& operator++() noexcept {
            if (current) current = current->ai_next;
            return *this;
        }
        
        bool operator!=(const iterator& other) const noexcept {
            return current != other.current;
        }
    };

    /**
     * @brief Get iterator to beginning
     * @return Iterator to first addrinfo, or end() if moved-from
     */
    iterator begin() const noexcept {
        return iterator(isValid() ? addrInfo_ : nullptr);
    }

    /**
     * @brief Get iterator to end
     * @return Iterator to end (always nullptr)
     */
    iterator end() const noexcept {
        return iterator(nullptr);
    }
};

/**
 * @brief Convenience function to get addrinfo with RAII protection
 * @param node Hostname or service name
 * @param service Service name or port number
 * @param hints Hints for getaddrinfo
 * @return AddrInfoGuard containing the result
 */
inline AddrInfoGuard GetAddrInfo(const char* node, const char* service, const struct addrinfo* hints) {
    struct addrinfo* result = nullptr;
    int ret = getaddrinfo(node, service, hints, &result);
    
    if (ret != 0 || result == nullptr) {
        return AddrInfoGuard(nullptr, false); // Return empty guard on error
    }
    
    return AddrInfoGuard(result, true);
}

} // namespace WebSocket

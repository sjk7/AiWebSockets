#pragma once

#include "Types.h"

namespace nob {

// Forward declarations for firewall compliance
struct addrinfo;

/**
 * @brief RAII wrapper for addrinfo structures
 * 
 * This class provides automatic memory management for addrinfo structures
 * returned by getaddrinfo(). It follows RAII principles and is move-only
 * to prevent accidental copying which would lead to double-free issues.
 * 
 * NOTE: All native socket operations are handled in AddrInfo.cpp to maintain
 * the compilation firewall. This header only contains the RAII interface.
 */
class AddrInfo {
private:
    struct addrinfo* addrInfo_;
    bool owns_;

public:
    /**
     * @brief Construct from raw addrinfo pointer
     * @param addrInfo Raw pointer from getaddrinfo()
     * @param owns Whether this guard owns the pointer (default: true)
     */
    explicit AddrInfo(struct addrinfo* addrInfo = nullptr, bool owns = true) noexcept
        : addrInfo_(addrInfo), owns_(owns) {}

    /**
     * @brief Destructor - automatically frees the addrinfo if owned and not moved-from
     */
    ~AddrInfo() noexcept;

    // Delete copy constructor and copy assignment
    AddrInfo(const AddrInfo&) = delete;
    AddrInfo& operator=(const AddrInfo&) = delete;

    /**
     * @brief Move constructor
     * @param other Guard to move from
     */
    AddrInfo(AddrInfo&& other) noexcept
        : addrInfo_(other.addrInfo_), owns_(other.owns_) {
        other.addrInfo_ = nullptr;
        other.owns_ = false;
    }

    /**
     * @brief Move assignment operator
     * @param other Guard to move from
     * @return Reference to this
     */
    AddrInfo& operator=(AddrInfo&& other) noexcept;

    /**
     * @brief Get the raw addrinfo pointer
     * @return Raw pointer to addrinfo structure
     */
    struct addrinfo* get() const noexcept { return addrInfo_; }

    /**
     * @brief Check if the guard contains a valid addrinfo
     * @return True if addrinfo is not null
     */
    bool isValid() const noexcept { return addrInfo_ != nullptr; }

    /**
     * @brief Release ownership of the addrinfo pointer
     * @return Raw pointer (caller takes ownership)
     */
    struct addrinfo* release() noexcept {
        owns_ = false;
        return addrInfo_;
    }

    /**
     * @brief Iterator for traversing addrinfo linked list
     */
    class iterator {
    private:
        struct addrinfo* current_;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = struct addrinfo;
        using difference_type = std::ptrdiff_t;
        using pointer = struct addrinfo*;
        using reference = struct addrinfo&;

        explicit iterator(struct addrinfo* ptr) noexcept : current_(ptr) {}

        reference operator*() const noexcept { return *current_; }
        pointer operator->() const noexcept { return current_; }

        iterator& operator++() noexcept {
            if (current_) current_ = current_->ai_next;
            return *this;
        }

        iterator operator++(int) noexcept {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const iterator& a, const iterator& b) noexcept {
            return a.current_ == b.current_;
        }

        friend bool operator!=(const iterator& a, const iterator& b) noexcept {
            return a.current_ != b.current_;
        }
    };

    /**
     * @brief Get iterator to the beginning of the addrinfo list
     */
    iterator begin() const noexcept {
        return iterator(addrInfo_);
    }

    /**
     * @brief Get iterator to the end of the addrinfo list
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
 * @return AddrInfo containing the result
 */
AddrInfo GetAddrInfo(const char* node, const char* service, const struct addrinfo* hints);

} // namespace nob

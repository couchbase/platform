/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <openssl/crypto.h>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace cb::crypto {

/**
 * A generic owning container for secret/sensitive data (e.g. passwords, keys,
 * tokens) that guarantees the stored bytes are overwritten with zeros when the
 * object is destroyed or when the secret is cleared explicitly.
 *
 * Copying is intentionally disabled to prevent accidental duplication of
 * sensitive material. Moving transfers ownership and leaves the source in a
 * wiped, empty state.
 *
 * An implicit conversion to std::string_view is provided so that a Secret<T>
 * can be passed directly to any API that accepts std::string_view without an
 * extra .data()/.size() call.
 *
 * Typical usage:
 * @code
 *   cb::crypto::Secret<std::string> password{"s3cr3t!"};
 *   cb::crypto::Secret<std::vector<unsigned char>> key{keyBytes};
 *
 *   // Pass to an API that takes std::string_view:
 *   cipher.encrypt(plaintext, password);   // implicit conversion
 * @endcode
 *
 * @tparam Container  Any contiguous-range type that exposes data(), size(),
 *                    and a value_type of char or unsigned char (e.g.
 *                    std::string, std::vector<char>,
 *                    std::vector<unsigned char>).
 */
template <typename Container>
class Secret {
public:
    using value_type = typename Container::value_type;

    static_assert(sizeof(value_type) == 1,
                  "Secret<> only supports byte-wide element types "
                  "(char, unsigned char, std::byte, …)");

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------

    /// Default-construct an empty secret.
    Secret() = default;

    /// Construct from an existing container (copy).
    explicit Secret(const Container& data) : data_(data) {
    }

    /// Construct from an rvalue container (move, no extra allocation).
    explicit Secret(Container&& data) noexcept : data_(std::move(data)) {
    }

    /// Construct from a std::string_view (copies the bytes).
    explicit Secret(std::string_view sv)
        : data_(reinterpret_cast<const value_type*>(sv.data()),
                reinterpret_cast<const value_type*>(sv.data()) + sv.size()) {
    }

    // ------------------------------------------------------------------
    // Copy — disabled
    // ------------------------------------------------------------------

    Secret(const Secret&) = delete;
    Secret& operator=(const Secret&) = delete;

    // ------------------------------------------------------------------
    // Move
    // ------------------------------------------------------------------

    /// Move-construct: transfer ownership and wipe the source.
    Secret(Secret&& other) noexcept : data_(std::move(other.data_)) {
        other.wipe();
    }

    /// Move-assign: wipe this first, then transfer ownership and wipe source.
    Secret& operator=(Secret&& other) noexcept {
        if (this != &other) {
            wipe();
            data_ = std::move(other.data_);
            other.wipe();
        }
        return *this;
    }

    // ------------------------------------------------------------------
    // Destruction
    // ------------------------------------------------------------------

    ~Secret() {
        wipe();
    }

    // ------------------------------------------------------------------
    // Observers
    // ------------------------------------------------------------------

    /// Returns a pointer to the raw bytes.
    [[nodiscard]] const value_type* data() const noexcept {
        return data_.data();
    }

    /// Returns the number of bytes stored.
    [[nodiscard]] std::size_t size() const noexcept {
        return data_.size();
    }

    /// Returns true if the secret contains no bytes.
    [[nodiscard]] bool empty() const noexcept {
        return data_.empty();
    }

    // ------------------------------------------------------------------
    // Implicit conversion to std::string_view
    // ------------------------------------------------------------------

    /**
     * Implicit conversion to std::string_view.
     *
     * Allows a Secret to be passed directly to any function that accepts
     * std::string_view, e.g.:
     *   void verify(std::string_view password);
     *   verify(mySecret);   // works without an explicit cast
     */
    operator std::string_view() const noexcept {
        return {reinterpret_cast<const char*>(data_.data()), data_.size()};
    }

    // ------------------------------------------------------------------
    // Mutation
    // ------------------------------------------------------------------

    /**
     * Securely overwrite and clear the stored data.
     * After calling this the secret is in the same state as a
     * default-constructed Secret.
     */
    void clear() noexcept {
        wipe();
    }

private:
    /// Overwrite the stored bytes then release the storage.
    void wipe() noexcept {
        if (!data_.empty()) {
            OPENSSL_cleanse(data_.data(), data_.size() * sizeof(value_type));
        }
        data_ = Container{};
    }

    Container data_{};
};

// ---------------------------------------------------------------------------
// Convenience aliases
// ---------------------------------------------------------------------------

/// Secret backed by a std::string (most common use-case: passwords, tokens).
using SecretString = Secret<std::string>;

/// Secret backed by a byte vector (use for binary keys / raw key material).
using SecretBytes = Secret<std::vector<unsigned char>>;

} // namespace cb::crypto

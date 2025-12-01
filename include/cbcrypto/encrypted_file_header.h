/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <cbcrypto/common.h>
#include <platform/uuid.h>
#include <array>
#include <cstdint> // uint8_t
#include <string_view>

namespace cb::crypto {
enum class Compression : uint8_t {
    None = 0,
    Snappy = 1,
    ZLIB = 2,
    GZIP = 3,
    ZSTD = 4,
    BZIP2 = 5,
};

std::string format_as(Compression compression);

/// Class representing the fixed size Encrypted Files
class EncryptedFileHeader {
public:
    constexpr static std::string_view Magic = {"\0Couchbase Encrypted\0", 21};
    /// Initialize a new instance of the FileHeader and set the key id to use
    explicit EncryptedFileHeader(std::string_view key_id,
                                 KeyDerivationMethod key_derivation,
                                 Compression compression,
                                 cb::uuid::uuid_t salt = cb::uuid::random());

    /// Is "this" an encrypted header (contains the correct magic)
    [[nodiscard]] bool is_encrypted() const;
    /// Is "this" encrypted and the version is something we support
    [[nodiscard]] bool is_supported() const;
    /// Get the compression type used in the file
    [[nodiscard]] Compression get_compression() const;
    /// Get the key derivation method used in the file
    [[nodiscard]] KeyDerivationMethod get_key_derivation() const;
    /// Get the number of iterations for PBKDF2
    [[nodiscard]] unsigned int get_pbkdf_iterations() const;
    /// Set the number of iterations for PBKDF2
    /// @param target the desired number of iterations
    /// @return the actual number set
    unsigned int set_pbkdf_iterations(unsigned int target);
    /// Get the key identifier in the header
    [[nodiscard]] std::string_view get_id() const;
    /// Get the salt used in the encryption
    [[nodiscard]] cb::uuid::uuid_t get_salt() const;
    /// Convenience function to convert to string_view
    [[nodiscard]] operator std::string_view() const {
        return {reinterpret_cast<const char*>(this), sizeof(*this)};
    }

    std::string derive_key(const KeyDerivationKey& kdk) const;

protected:
    std::array<char, 21> magic{};
    uint8_t version{0};
    uint8_t compression{0};
    uint8_t key_derivation{0};
    std::array<uint8_t, 3> unused{};
    uint8_t id_size = 0;
    std::array<char, 36> id{};
    std::array<uint8_t, 16> salt{};
};

static_assert(sizeof(EncryptedFileHeader) == 80);
} // namespace cb::crypto

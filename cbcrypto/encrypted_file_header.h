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
#include <array>
#include <cstdint> // uint8_t
#include <string_view>

/// Class representing the fixed size Encrypted Files
class EncryptedFileHeader {
public:
    constexpr static std::string_view Magic = {"\0Couchbase Encrypted File\0",
                                               26};
    /// Initialize a new instance of the FileHeader and set the key id to
    /// use
    explicit EncryptedFileHeader(std::string_view key_id);

    /// Is "this" an encrypted header (contains the correct magic)
    [[nodiscard]] bool is_encrypted() const;
    /// Is "this" encrypted and the version is something we support
    [[nodiscard]] bool is_supported() const;
    /// Get the key identifier in the header
    [[nodiscard]] std::string_view get_id() const;

    /// Convenience function to convert to string_view
    [[nodiscard]] operator std::string_view() const {
        return {reinterpret_cast<const char*>(this), sizeof(*this)};
    }

protected:
    std::array<char, 26> magic{};
    uint8_t version{0};
    uint8_t id_size = 0;
    std::array<char, 36> id{};
};

static_assert(sizeof(EncryptedFileHeader) == 64);

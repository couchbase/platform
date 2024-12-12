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

#include <cbcrypto/encrypted_file_header.h>
#include <platform/platform_socket.h>

namespace cb::crypto {

class EncryptedFileAssociatedData {
public:
    EncryptedFileAssociatedData(EncryptedFileHeader header) : header(header) {
    }
    void set_offset(uint64_t value) {
        offset = htonll(value);
    }

    /// Convenience function to convert to string_view
    [[nodiscard]] operator std::string_view() const {
        return {reinterpret_cast<const char*>(this), sizeof(*this)};
    }

protected:
#pragma pack(1)
    EncryptedFileHeader header;
    uint64_t offset{0};
#pragma pack()
};

static_assert(sizeof(EncryptedFileAssociatedData) == 88,
              "Incorrect compiler padding");

} // namespace cb::crypto

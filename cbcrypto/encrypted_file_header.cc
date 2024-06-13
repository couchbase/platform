/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "encrypted_file_header.h"
#include <gsl/gsl-lite.hpp>

EncryptedFileHeader::EncryptedFileHeader(std::string_view key_id)
    : id_size(gsl::narrow_cast<uint8_t>(key_id.size())) {
    if (key_id.size() > id.size()) {
        throw std::invalid_argument("FileHeader::FileHeader(): key too long");
    }
    std::copy(Magic.begin(), Magic.end(), magic.begin());
    std::copy(key_id.begin(), key_id.end(), id.begin());
}

bool EncryptedFileHeader::is_encrypted() const {
    const auto header = std::string_view{magic.data(), magic.size()};
    return header == Magic;
}

bool EncryptedFileHeader::is_supported() const {
    return is_encrypted() && version == 0;
}

std::string_view EncryptedFileHeader::get_id() const {
    return std::string_view{id.data(), id_size};
}

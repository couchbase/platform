/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <cbcrypto/encrypted_file_header.h>
#include <fmt/format.h>
#include <gsl/gsl-lite.hpp>

namespace cb::crypto {

std::string format_as(Compression compression) {
    switch (compression) {
    case Compression::None:
        return "None";
    case Compression::Snappy:
        return "Snappy";
    case Compression::ZLIB:
        return "ZLIB";
    case Compression::GZIP:
        return "GZIP";
    case Compression::ZSTD:
        return "ZSTD";
    case Compression::BZIP2:
        return "BZIP2";
    }
    return fmt::format("Unknown-{}", static_cast<int>(compression));
}

EncryptedFileHeader::EncryptedFileHeader(std::string_view key_id,
                                         cb::uuid::uuid_t salt_,
                                         Compression compression)
    : compression(static_cast<uint8_t>(compression)),
      id_size(gsl::narrow_cast<uint8_t>(key_id.size())) {
    if (key_id.size() > id.size()) {
        throw std::invalid_argument("FileHeader::FileHeader(): key too long");
    }

    std::copy(Magic.begin(), Magic.end(), magic.begin());
    std::copy(key_id.begin(), key_id.end(), id.begin());
    std::copy(salt_.data, salt_.data + salt_.size(), salt.begin());
}

bool EncryptedFileHeader::is_encrypted() const {
    const auto header = std::string_view{magic.data(), magic.size()};
    return header == Magic;
}

bool EncryptedFileHeader::is_supported() const {
    return is_encrypted() && version == 0;
}

Compression EncryptedFileHeader::get_compression() const {
    return static_cast<Compression>(compression);
}

std::string_view EncryptedFileHeader::get_id() const {
    return std::string_view{id.data(), id_size};
}

cb::uuid::uuid_t EncryptedFileHeader::get_salt() const {
    cb::uuid::uuid_t ret;
    std::copy(salt.begin(), salt.end(), ret.data);
    return ret;
}

} // namespace cb::crypto

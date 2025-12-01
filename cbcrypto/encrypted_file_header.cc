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

#include <cbcrypto/digest.h>
#include <cbcrypto/key_derivation.h>
#include <cbcrypto/symmetric.h>
#include <fmt/format.h>
#include <gsl/gsl-lite.hpp>

namespace cb::crypto {

/// Value for Label when deriving encryption key for Couchbase Encrypted File
constexpr auto CEF_KDF_LABEL = "Couchbase File Encryption";
/// Prefix for Context when deriving encryption key for Couchbase Encrypted File
constexpr auto CEF_KDF_CONTEXT = "Couchbase Encrypted File/";
/// Multiplier for calculating the number of PBKDF2 iterations
constexpr unsigned int CEF_ITERATION_MULTIPLIER = 1024;

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
                                         KeyDerivationMethod key_derivation_,
                                         Compression compression_,
                                         cb::uuid::uuid_t salt_)
    : compression(static_cast<uint8_t>(compression_)),
      key_derivation(static_cast<uint8_t>(key_derivation_)),
      id_size(gsl::narrow_cast<uint8_t>(key_id.size())) {
    if (key_id.size() > id.size()) {
        throw std::invalid_argument("FileHeader::FileHeader(): key too long");
    }
    if (key_derivation_ != KeyDerivationMethod::NoDerivation) {
        version = 1;
    }
    std::copy(Magic.begin(), Magic.end(), magic.begin());
    std::copy(key_id.begin(), key_id.end(), id.begin());
    std::copy(salt_.begin(), salt_.end(), salt.begin());
}

bool EncryptedFileHeader::is_encrypted() const {
    const auto header = std::string_view{magic.data(), magic.size()};
    return header == Magic;
}

bool EncryptedFileHeader::is_supported() const {
    return version <= 1 && is_encrypted();
}

Compression EncryptedFileHeader::get_compression() const {
    return static_cast<Compression>(compression);
}

KeyDerivationMethod EncryptedFileHeader::get_key_derivation() const {
    return static_cast<KeyDerivationMethod>(key_derivation & 0xf);
}

unsigned int EncryptedFileHeader::get_pbkdf_iterations() const {
    return CEF_ITERATION_MULTIPLIER << (key_derivation >> 4);
}

unsigned int EncryptedFileHeader::set_pbkdf_iterations(unsigned int target) {
    Expects(get_key_derivation() == KeyDerivationMethod::PasswordBased);
    auto count = CEF_ITERATION_MULTIPLIER;
    uint8_t value = 0;
    while (count < target && value < 0xf0) {
        count <<= 1;
        value += 0x10;
    }
    key_derivation = (key_derivation & 0xf) | value;
    return count;
}

std::string_view EncryptedFileHeader::get_id() const {
    return std::string_view{id.data(), id_size};
}

cb::uuid::uuid_t EncryptedFileHeader::get_salt() const {
    cb::uuid::uuid_t ret;
    std::copy(salt.begin(), salt.end(), ret.begin());
    return ret;
}

std::string EncryptedFileHeader::derive_key(
        const cb::crypto::KeyDerivationKey& kdk) const {
    if (version == 0) {
        return kdk.derivationKey;
    }
    if (version != 1) {
        throw std::invalid_argument(
                "cb::crypto::EncryptedFileHeader::derive_key: invalid version");
    }
    switch (get_key_derivation()) {
    case KeyDerivationMethod::NoDerivation:
        return kdk.derivationKey;
    case KeyDerivationMethod::KeyBased:
        return deriveKey(SymmetricCipher::getKeySize(kdk.cipher),
                         kdk.derivationKey,
                         CEF_KDF_LABEL,
                         CEF_KDF_CONTEXT + ::to_string(get_salt()));
    case KeyDerivationMethod::PasswordBased:
        return PBKDF2_HMAC(Algorithm::SHA256,
                           kdk.derivationKey,
                           CEF_KDF_CONTEXT + ::to_string(get_salt()),
                           get_pbkdf_iterations());
    }
    throw std::invalid_argument(
            "cb::crypto::EncryptedFileHeader::derive_key: invalid kdf");
}

} // namespace cb::crypto

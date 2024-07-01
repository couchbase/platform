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

#include <gsl/gsl-lite.hpp>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>
#include <string>

namespace cb::crypto {

enum class Cipher {
    /// Special value indicating no cipher
    None,
    AES_256_GCM
};

[[nodiscard]] std::string format_as(const Cipher& cipher);
[[nodiscard]] Cipher to_cipher(std::string_view name);
void to_json(nlohmann::json& json, const Cipher& cipher);
void from_json(const nlohmann::json& json, Cipher& cipher);

/// EncryptionKeyIface defines an interface for the encryption key.
class EncryptionKeyIface {
public:
    virtual ~EncryptionKeyIface() = default;

    /// The identifier for the encryption key.
    virtual std::string_view getId() const = 0;

    /// The cipher used for the key
    virtual Cipher getCipher() const = 0;

    /// The actual key
    virtual std::string_view getKey() const = 0;
};

/// A structure to hold the information needed by a single key
struct DataEncryptionKey final : public EncryptionKeyIface {
    /// generate a key with the provided cipher type
    static std::unique_ptr<DataEncryptionKey> generate(
            Cipher cipher_type = Cipher::AES_256_GCM);

    DataEncryptionKey() = default;

    DataEncryptionKey(std::string id, Cipher cipher, std::string key);

    ~DataEncryptionKey() override = default;

    std::string_view getId() const override;

    Cipher getCipher() const override;

    std::string_view getKey() const override;

    /// The identification for the current key
    std::string id;
    /// The cipher used for the key
    Cipher cipher{Cipher::None};
    /// The actual key
    std::string key;

    bool operator==(const DataEncryptionKey&) const;
};

[[nodiscard]] std::string format_as(const DataEncryptionKey& dek);
void to_json(nlohmann::json& json, const DataEncryptionKey& dek);
void from_json(const nlohmann::json& json, DataEncryptionKey& dek);
using SharedEncryptionKey = std::shared_ptr<const DataEncryptionKey>;

class NotSupportedException : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

class OpenSslError : public std::runtime_error {
public:
    static OpenSslError get(const char* callingFunction,
                            const char* openSslFunction);

    OpenSslError(const char* callingFunction,
                 const char* openSslFunction,
                 unsigned long errorCode,
                 std::string message);

    const char* const callingFunction;

    const char* const openSslFunction;

    const unsigned long errorCode;

    const std::string message;
};

/**
 * Populate `buf` with random bytes from an OpenSSL DRBG
 */
void randomBytes(gsl::span<char> buf);

} // namespace cb::crypto

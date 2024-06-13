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

/// A structure to hold the information needed by a single key
struct DataEncryptionKey {
    /// generate a key with the provided cipher type
    static std::shared_ptr<DataEncryptionKey> generate(
            std::string_view cipher_string = "AES-256-GCM");

    /// The identification for the current key
    std::string id;
    /// The cipher used for the key
    std::string cipher;
    /// The actual key
    std::string key;
};

[[nodiscard]] std::string format_as(const DataEncryptionKey& dek);
void to_json(nlohmann::json& json, const DataEncryptionKey& dek);
void from_json(const nlohmann::json& json, DataEncryptionKey& dek);

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
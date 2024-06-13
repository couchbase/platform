/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "cbcrypto/common.h"
#include "cbcrypto/symmetric.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <platform/base64.h>
#include <platform/uuid.h>
#include <array>

namespace cb::crypto {

std::string format_as(const DataEncryptionKey& dek) {
    nlohmann::json json = dek;
    json.erase("key");
    return json.dump();
}

std::shared_ptr<DataEncryptionKey> DataEncryptionKey::generate(
        std::string_view cipher_string) {
    auto ret = std::make_shared<DataEncryptionKey>();
    ret->id = to_string(uuid::random());
    ret->cipher = cipher_string;
    ret->key = SymmetricCipher::generateKey(cipher_string);
    return ret;
}

void to_json(nlohmann::json& json, const DataEncryptionKey& dek) {
    json = {{"id", dek.id},
            {"cipher", dek.cipher},
            {"key", base64::encode(dek.key)}};
}

void from_json(const nlohmann::json& json, DataEncryptionKey& dek) {
    dek.id = json["id"].get<std::string>();
    dek.cipher = json["cipher"].get<std::string>();
    dek.key = base64::decode(json["key"].get<std::string>());
}

OpenSslError OpenSslError::get(const char* callingFunction,
                               const char* openSslFunction) {
    std::array<char, 256> msg;
    const auto code = ERR_get_error();
    ERR_error_string_n(code, msg.data(), msg.size() - 1);
    return {callingFunction, openSslFunction, code, msg.data()};
}

OpenSslError::OpenSslError(const char* callingFunction,
                           const char* openSslFunction,
                           unsigned long errorCode,
                           std::string message)
    : std::runtime_error(fmt::format("{}: OpenSSL {} failed: {}",
                                     callingFunction,
                                     openSslFunction,
                                     message)),
      callingFunction(callingFunction),
      openSslFunction(openSslFunction),
      errorCode(errorCode),
      message(std::move(message)) {
}

void randomBytes(gsl::span<char> buf) {
    if (RAND_bytes(reinterpret_cast<unsigned char*>(buf.data()), buf.size()) !=
        1) {
        throw OpenSslError::get("cb::crypto::randomBytes", "RAND_bytes");
    }
}

} // namespace cb::crypto

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

[[nodiscard]] std::string format_as(const Cipher& cipher) {
    switch (cipher) {
    case Cipher::None:
        return "None";
    case Cipher::AES_256_GCM:
        return "AES-256-GCM";
    }
    throw std::invalid_argument(
            fmt::format("format_as(const Cipher& cipher): unknown cipher: {}",
                        static_cast<int>(cipher)));
}

[[nodiscard]] Cipher to_cipher(std::string_view name) {
    if (name == "AES-256-GCM") {
        return Cipher::AES_256_GCM;
    }
    if (name == "None") {
        return Cipher::None;
    }

    throw std::invalid_argument(fmt::format(
            "to_cipher(std::string_view name): unknown cipher: {}", name));
}

void to_json(nlohmann::json& json, const Cipher& cipher) {
    json = format_as(cipher);
}

void from_json(const nlohmann::json& json, Cipher& cipher) {
    cipher = to_cipher(json.get<std::string>());
}

std::string format_as(const DataEncryptionKey& dek) {
    nlohmann::json json = dek;
    json.erase("key");
    return json.dump();
}

std::unique_ptr<DataEncryptionKey> DataEncryptionKey::generate(
        Cipher cipher_type) {
    Expects(cipher_type != Cipher::None);
    auto ret = std::make_unique<DataEncryptionKey>();
    ret->id = to_string(uuid::random());
    ret->cipher = cipher_type;
    ret->key = SymmetricCipher::generateKey(ret->cipher);
    Expects(ret->key.size() == SymmetricCipher::getKeySize(ret->cipher));
    return ret;
}

void to_json(nlohmann::json& json, const DataEncryptionKey& dek) {
    json = {{"id", dek.id},
            {"cipher", dek.cipher},
            {"key", base64::encode(dek.key)}};
}

void from_json(const nlohmann::json& json, DataEncryptionKey& dek) {
    dek.id = json["id"].get<std::string>();
    dek.cipher = json["cipher"];
    dek.key = base64::decode(json["key"].get<std::string>());
    Expects(dek.key.size() == SymmetricCipher::getKeySize(dek.cipher));
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

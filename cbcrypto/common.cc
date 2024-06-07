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

#include <fmt/format.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <array>

cb::crypto::OpenSslError cb::crypto::OpenSslError::get(
        const char* callingFunction, const char* openSslFunction) {
    std::array<char, 256> msg;
    const auto code = ERR_get_error();
    ERR_error_string_n(code, msg.data(), msg.size() - 1);
    return {callingFunction, openSslFunction, code, msg.data()};
}

cb::crypto::OpenSslError::OpenSslError(const char* callingFunction,
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

void cb::crypto::randomBytes(gsl::span<char> buf) {
    if (RAND_bytes(reinterpret_cast<unsigned char*>(buf.data()), buf.size()) !=
        1) {
        throw OpenSslError::get("cb::crypto::randomBytes", "RAND_bytes");
    }
}

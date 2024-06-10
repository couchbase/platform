/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "cbcrypto/random_gen.h"

#include <openssl/evp.h>
#include <openssl/params.h>

namespace cb::crypto {
namespace internal {

struct EvpRandDeleter {
    void operator()(EVP_RAND* ptr) {
        EVP_RAND_free(ptr);
    }
};

struct EvpRandCtxDeleter {
    void operator()(EVP_RAND_CTX* ptr) {
        EVP_RAND_CTX_free(ptr);
    }
};

class HashDrbgSha256 final : public RandomBitGenerator {
public:
    HashDrbgSha256(const char* properties);

    void generate(gsl::span<char> buf) override;

protected:
    std::unique_ptr<EVP_RAND, EvpRandDeleter> evpRand;

    std::unique_ptr<EVP_RAND_CTX, EvpRandCtxDeleter> evpRandCtx;
};

HashDrbgSha256::HashDrbgSha256(const char* properties)
    : evpRand(EVP_RAND_fetch(nullptr, "HASH-DRBG", properties)) {
    if (!evpRand) {
        throw OpenSslError::get("cb::crypto::HashDrbgSha256::HashDrbgSha256",
                                "EVP_RAND_fetch");
    }
    evpRandCtx.reset(EVP_RAND_CTX_new(evpRand.get(), nullptr));
    if (!evpRandCtx) {
        throw OpenSslError::get("cb::crypto::HashDrbgSha256::HashDrbgSha256",
                                "EVP_RAND_CTX_new");
    }
    std::array<OSSL_PARAM, 2> params{
            {OSSL_PARAM_construct_utf8_string(
                     "digest", const_cast<char*>("SHA-256"), 7),
             OSSL_PARAM_construct_end()}};
    if (EVP_RAND_instantiate(
                evpRandCtx.get(), 256, 0, nullptr, 0, params.data()) != 1) {
        throw OpenSslError::get("cb::crypto::HashDrbgSha256::HashDrbgSha256",
                                "EVP_RAND_instantiate");
    }
}

void HashDrbgSha256::generate(gsl::span<char> buf) {
    if (EVP_RAND_generate(evpRandCtx.get(),
                          reinterpret_cast<unsigned char*>(buf.data()),
                          buf.size(),
                          256,
                          0,
                          nullptr,
                          0) != 1) {
        throw OpenSslError::get("cb::crypto::HashDrbgSha256::generate",
                                "EVP_RAND_generate");
    }
}

} // namespace internal

std::unique_ptr<RandomBitGenerator> RandomBitGenerator::create(
        const char* properties) {
    return std::make_unique<internal::HashDrbgSha256>(properties);
}

} // namespace cb::crypto

/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cbcrypto/key_derivation.h>

#include <cbcrypto/common.h>

#include <fmt/format.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>

namespace cb::crypto {

struct EvpKdfDeleter {
    void operator()(EVP_KDF* ptr) const {
        EVP_KDF_free(ptr);
    }

    void operator()(EVP_KDF_CTX* ptr) const {
        EVP_KDF_CTX_free(ptr);
    }
};

std::string deriveKey(std::size_t derivedSize,
                      std::string_view kdk,
                      std::string_view label,
                      std::string_view context,
                      KeyDerivationFunction kdf) {
    Expects(!kdk.empty());

    std::string derived(derivedSize, '\0');
    int useL =
            kdf == KeyDerivationFunction::HMAC_SHA256_COUNTER_WITHOUT_L ? 0 : 1;
    const std::array<OSSL_PARAM, 7> params = {
            OSSL_PARAM_construct_utf8_string(
                    OSSL_KDF_PARAM_MAC,
                    const_cast<char*>(OSSL_MAC_NAME_HMAC),
                    0),
            OSSL_PARAM_construct_utf8_string(
                    OSSL_KDF_PARAM_DIGEST,
                    const_cast<char*>(OSSL_DIGEST_NAME_SHA2_256),
                    0),
            OSSL_PARAM_construct_octet_string(
                    OSSL_KDF_PARAM_KEY,
                    reinterpret_cast<void*>(const_cast<char*>(kdk.data())),
                    kdk.size()),
            // parameter salt is the label for KBKDF.
            OSSL_PARAM_construct_octet_string(
                    OSSL_KDF_PARAM_SALT,
                    reinterpret_cast<void*>(const_cast<char*>(label.data())),
                    label.size()),
            // parameter info is the context for KBKDF.
            OSSL_PARAM_construct_octet_string(
                    OSSL_KDF_PARAM_INFO,
                    reinterpret_cast<void*>(const_cast<char*>(context.data())),
                    context.size()),
            OSSL_PARAM_construct_int(OSSL_KDF_PARAM_KBKDF_USE_L, &useL),
            OSSL_PARAM_END};

    std::unique_ptr<EVP_KDF, EvpKdfDeleter> evpKdf(
            EVP_KDF_fetch(nullptr, "KBKDF", nullptr));
    if (!evpKdf) {
        throw OpenSslError::get("cb::crypto::deriveKey", "EVP_KDF_fetch");
    }
    std::unique_ptr<EVP_KDF_CTX, EvpKdfDeleter> ctx(
            EVP_KDF_CTX_new(evpKdf.get()));
    if (!ctx) {
        throw OpenSslError::get("cb::crypto::deriveKey", "EVP_KDF_CTX_new");
    }
    if (EVP_KDF_derive(ctx.get(),
                       reinterpret_cast<unsigned char*>(derived.data()),
                       derived.size(),
                       params.data()) != 1) {
        throw OpenSslError::get("cb::crypto::deriveKey", "EVP_KDF_derive");
    }

    return derived;
}

} // namespace cb::crypto

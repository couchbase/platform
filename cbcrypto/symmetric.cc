/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "cbcrypto/symmetric.h"

#include <fmt/format.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>

#include <memory>

using namespace std::string_view_literals;

namespace cb::crypto {
namespace internal {

struct EvpCipherDeleter {
    void operator()(EVP_CIPHER* ptr) {
        EVP_CIPHER_free(ptr);
    }
};
using EvpCipherUniquePtr = std::unique_ptr<EVP_CIPHER, EvpCipherDeleter>;

struct EvpCipherCtxDeleter {
    void operator()(EVP_CIPHER_CTX* ptr) {
        EVP_CIPHER_CTX_free(ptr);
    }
};

using EvpCipherCtxUniquePtr =
        std::unique_ptr<EVP_CIPHER_CTX, EvpCipherCtxDeleter>;

class Aes256Gcm final : public SymmetricCipher {
public:
    Aes256Gcm(std::string key, const char* properties);

    void encrypt(std::string_view nonce,
                 gsl::span<char> ct,
                 gsl::span<char> mac,
                 std::string_view msg,
                 std::string_view ad) override;

    void decrypt(std::string_view nonce,
                 std::string_view ct,
                 std::string_view mac,
                 gsl::span<char> msg,
                 std::string_view ad) override;

    std::size_t getNonceSize() const override {
        return NonceSize;
    }

    std::size_t getMacSize() const override {
        return MacSize;
    }

    static constexpr std::size_t KeySize = 32;
    static constexpr std::size_t NonceSize = 12;
    static constexpr std::size_t MacSize = 16;

protected:
    const std::unique_ptr<EVP_CIPHER, EvpCipherDeleter> cipher;

    const std::string key;
};

Aes256Gcm::Aes256Gcm(std::string key, const char* properties)
    : cipher(EVP_CIPHER_fetch(nullptr, "AES-256-GCM", properties)),
      key(std::move(key)) {
    Expects(this->key.size() == KeySize);
    if (!cipher) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::Aes256Gcm",
                                "EVP_CIPHER_fetch");
    }
}

void Aes256Gcm::encrypt(std::string_view nonce,
                        gsl::span<char> ct,
                        gsl::span<char> mac,
                        std::string_view msg,
                        std::string_view ad) {
    Expects(nonce.size() == NonceSize);
    Expects(mac.size() == MacSize);
    Expects(ct.size() == msg.size());
    int outlen = 0;
    EvpCipherCtxUniquePtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::encrypt",
                                "EVP_CIPHER_CTX_new");
    }
    std::array<OSSL_PARAM, 2> params{{OSSL_PARAM_END, OSSL_PARAM_END}};
    if (EVP_EncryptInit_ex2(
                ctx.get(),
                cipher.get(),
                reinterpret_cast<const unsigned char*>(key.data()),
                reinterpret_cast<const unsigned char*>(nonce.data()),
                params.data()) != 1) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::encrypt",
                                "EVP_EncryptInit_ex2");
    }
    if (!ad.empty()) {
        if (EVP_EncryptUpdate(ctx.get(),
                              nullptr,
                              &outlen,
                              reinterpret_cast<const unsigned char*>(ad.data()),
                              ad.size()) != 1) {
            throw OpenSslError::get("cb::crypto::Aes256Gcm::encrypt",
                                    "EVP_EncryptUpdate(ad)");
        }
    }
    if (EVP_EncryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(ct.data()),
                          &outlen,
                          reinterpret_cast<const unsigned char*>(msg.data()),
                          msg.size()) != 1 ||
        static_cast<std::size_t>(outlen) != ct.size()) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::encrypt",
                                "EVP_EncryptUpdate(msg)");
    }
    unsigned char dummyBuffer; // No output expected
    if (EVP_EncryptFinal_ex(ctx.get(), &dummyBuffer, &outlen) != 1) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::encrypt",
                                "EVP_EncryptFinal_ex");
    }
    params[0] = OSSL_PARAM_construct_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG,
                                                  const_cast<char*>(mac.data()),
                                                  mac.size());
    if (EVP_CIPHER_CTX_get_params(ctx.get(), params.data()) != 1) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::encrypt",
                                "EVP_CIPHER_CTX_get_params");
    }
}

void Aes256Gcm::decrypt(std::string_view nonce,
                        std::string_view ct,
                        std::string_view mac,
                        gsl::span<char> msg,
                        std::string_view ad) {
    Expects(nonce.size() == NonceSize);
    Expects(mac.size() == MacSize);
    Expects(ct.size() == msg.size());
    int outlen = 0;
    EvpCipherCtxUniquePtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::decrypt",
                                "EVP_CIPHER_CTX_new");
    }
    std::array<OSSL_PARAM, 2> params{
            {OSSL_PARAM_construct_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG,
                                               const_cast<char*>(mac.data()),
                                               mac.size()),
             OSSL_PARAM_END}};
    if (EVP_DecryptInit_ex2(
                ctx.get(),
                cipher.get(),
                reinterpret_cast<const unsigned char*>(key.data()),
                reinterpret_cast<const unsigned char*>(nonce.data()),
                params.data()) != 1) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::decrypt",
                                "EVP_DecryptInit_ex2");
    }
    if (!ad.empty()) {
        if (EVP_DecryptUpdate(ctx.get(),
                              nullptr,
                              &outlen,
                              reinterpret_cast<const unsigned char*>(ad.data()),
                              ad.size()) != 1) {
            throw OpenSslError::get("cb::crypto::Aes256Gcm::decrypt",
                                    "EVP_DecryptUpdate(ad)");
        }
    }
    if (EVP_DecryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(msg.data()),
                          &outlen,
                          reinterpret_cast<const unsigned char*>(ct.data()),
                          ct.size()) != 1 ||
        static_cast<std::size_t>(outlen) != msg.size()) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::decrypt",
                                "EVP_DecryptUpdate(msg)");
    }
    unsigned char dummyBuffer; // No output expected
    if (EVP_DecryptFinal_ex(ctx.get(), &dummyBuffer, &outlen) != 1) {
        throw MacVerificationError(
                "cb::crypto::Aes256Gcm::decrypt: "
                "MAC verification failed");
    }
}

} // namespace internal

std::string SymmetricCipher::encrypt(std::string_view msg,
                                     std::string_view ad) {
    const auto nonceSize = getNonceSize();
    const auto macSize = getMacSize();
    std::string ret;
    ret.resize(nonceSize + msg.size() + macSize);
    gsl::span span(ret);

    // Generate random nonce
    randomBytes(span.subspan(0, nonceSize));

    encrypt({ret.data(), nonceSize},
            span.subspan(nonceSize, msg.size()),
            span.subspan(nonceSize + msg.size(), macSize),
            msg,
            ad);

    return ret;
}

std::string SymmetricCipher::decrypt(std::string_view ct, std::string_view ad) {
    const auto nonceSize = getNonceSize();
    const auto macSize = getMacSize();

    if (ct.size() < (nonceSize + macSize)) {
        throw std::runtime_error(
                "cb::crypto::SymmetricCipher::decrypt: "
                "Data too small to contain nonce and MAC");
    }

    std::string ret;
    ret.resize(ct.size() - nonceSize - macSize);

    decrypt(ct.substr(0, nonceSize),
            ct.substr(nonceSize, ret.size()),
            ct.substr(nonceSize + ret.size(), macSize),
            ret,
            ad);

    return ret;
}

std::string SymmetricCipher::generateKey(std::string_view cipher) {
    if (cipher == "AES-256-GCM"sv) {
        using namespace cb::crypto::internal;
        EvpCipherUniquePtr evp_cipher(
                EVP_CIPHER_fetch(nullptr, "AES-256-GCM", ""));
        if (!evp_cipher) {
            throw OpenSslError::get("cb::crypto::SymmetricCipher::generateKey",
                                    "EVP_CIPHER_fetch");
        }

        std::string ret;
        ret.resize(EVP_CIPHER_get_key_length(evp_cipher.get()));
        randomBytes(ret);
        return ret;
    }

    throw NotSupportedException(
            fmt::format("cb::crypto::SymmetricCipher::generateKey: "
                        "Cipher {} not supported",
                        cipher));
}

std::unique_ptr<SymmetricCipher> SymmetricCipher::create(
        std::string_view cipherName, std::string key, const char* properties) {
    if (cipherName == "AES-256-GCM"sv) {
        return std::make_unique<internal::Aes256Gcm>(std::move(key),
                                                     properties);
    }

    throw NotSupportedException(
            fmt::format("cb::crypto::SymmetricCipher::create: "
                        "Cipher {} not supported",
                        cipherName));
}

} // namespace cb::crypto

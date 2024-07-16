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
    Aes256Gcm(std::string_view key, const char* properties);

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

    std::array<char, KeySize> key;
};

Aes256Gcm::Aes256Gcm(std::string_view key, const char* properties)
    : cipher(EVP_CIPHER_fetch(nullptr, "AES-256-GCM", properties)) {
    Expects(key.size() == KeySize);
    if (!cipher) {
        throw OpenSslError::get("cb::crypto::Aes256Gcm::Aes256Gcm",
                                "EVP_CIPHER_fetch");
    }
    std::copy(key.begin(), key.end(), this->key.begin());
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

/**
 * Converts an unsigned integer (64-bit) to a big-endian value of the specified
 * size.
 *
 * The representation is stored in a fixed-size buffer, which limits the maximum
 * size.
 */
class SerializedUInt {
public:
    SerializedUInt(uint64_t value, size_t size) : len(size) {
        if (size > buffer.size()) {
            throw NotSupportedException(fmt::format(
                    "cb::crypto::SerializedUInt: Unsupported size:{}", size));
        }
        // Fill the buffer with the big-endian representation,
        // padding with zeros at the front if needed.
        for (auto ii = size; ii--;) {
            buffer[ii] = static_cast<unsigned char>(value);
            value >>= CHAR_BIT;
        }
        // The size should be large enough to contain the value
        Expects(value == 0);
    }

    const char* data() const {
        return reinterpret_cast<const char*>(buffer.data());
    }

    size_t size() const {
        return len;
    }

    operator std::string_view() const {
        return {data(), size()};
    }

protected:
    /// Buffer to store the serialized representation.
    /// May be larger than the specified size.
    std::array<unsigned char, 12> buffer;
    /// Size of the big-endian value.
    unsigned char len;
};

} // namespace internal

void SymmetricCipher::encrypt(std::uint64_t nonce,
                              gsl::span<char> ct,
                              gsl::span<char> mac,
                              std::string_view msg,
                              std::string_view ad) {
    internal::SerializedUInt serializedNonce{nonce, getNonceSize()};
    return encrypt(serializedNonce, ct, mac, msg, ad);
}

void SymmetricCipher::decrypt(std::uint64_t nonce,
                              std::string_view ct,
                              std::string_view mac,
                              gsl::span<char> msg,
                              std::string_view ad) {
    internal::SerializedUInt serializedNonce{nonce, getNonceSize()};
    return decrypt(serializedNonce, ct, mac, msg, ad);
}

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

std::string SymmetricCipher::generateKey(Cipher cipher) {
    if (cipher == Cipher::AES_256_GCM) {
        using namespace cb::crypto::internal;
        EvpCipherUniquePtr evp_cipher(
                EVP_CIPHER_fetch(nullptr, "AES-256-GCM", ""));
        if (!evp_cipher) {
            throw OpenSslError::get("cb::crypto::SymmetricCipher::generateKey",
                                    "EVP_CIPHER_fetch");
        }

        std::string ret;
        ret.resize(EVP_CIPHER_get_key_length(evp_cipher.get()));
        Expects(ret.size() == internal::Aes256Gcm::KeySize);
        randomBytes(ret);
        return ret;
    }

    throw NotSupportedException(
            fmt::format("cb::crypto::SymmetricCipher::generateKey: "
                        "Cipher {} not supported",
                        cipher));
}

std::unique_ptr<SymmetricCipher> SymmetricCipher::create(
        Cipher cipher, std::string_view key, const char* properties) {
    switch (cipher) {
    case Cipher::None:
        break;
    case Cipher::AES_256_GCM:
        Expects(key.size() == internal::Aes256Gcm::KeySize);
        return std::make_unique<internal::Aes256Gcm>(key, properties);
    }
    throw NotSupportedException(fmt::format(
            "cb::crypto::SymmetricCipher::create: Cipher {} not supported",
            cipher));
}

std::size_t SymmetricCipher::getKeySize(Cipher cipher) {
    switch (cipher) {
    case Cipher::None:
        return 0;
    case Cipher::AES_256_GCM:
        return internal::Aes256Gcm::KeySize;
    }
    throw std::invalid_argument(fmt::format(
            "SymmetricCipher::getKeySize(Cipher cipher): Unknown cipher: {}",
            static_cast<int>(cipher)));
}

std::size_t SymmetricCipher::getNonceSize(Cipher cipher) {
    switch (cipher) {
    case Cipher::None:
        return 0;
    case Cipher::AES_256_GCM:
        return internal::Aes256Gcm::NonceSize;
    }
    throw std::invalid_argument(fmt::format(
            "SymmetricCipher::getNonceSize(Cipher cipher): Unknown cipher: {}",
            static_cast<int>(cipher)));
}

std::size_t SymmetricCipher::getMacSize(Cipher cipher) {
    switch (cipher) {
    case Cipher::None:
        return 0;
    case Cipher::AES_256_GCM:
        return internal::Aes256Gcm::MacSize;
    }
    throw std::invalid_argument(fmt::format(
            "SymmetricCipher::getMacSize(Cipher cipher): Unknown cipher: {}",
            static_cast<int>(cipher)));
}

} // namespace cb::crypto

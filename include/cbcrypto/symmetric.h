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

#include "common.h"

#include <memory>

namespace cb::crypto {

class MacVerificationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * Cipher which uses the same key for encryption and decryption
 *
 * Intended for ciphers/modes that verify data integrity and
 * don't require padding.
 */
class SymmetricCipher {
public:
    /**
     * Instantiates a SymmetricCipher with the given name and key.
     *
     * @param cipherName OpenSSL name for the cipher
     * @param key Key to use
     * @param properties Properties to pass to OpenSSL (e.g. provider)
     */
    static std::unique_ptr<SymmetricCipher> create(
            std::string_view cipherName,
            std::string key,
            const char* properties = nullptr);

    /**
     * Encrypt `msg` by generating a random nonce.
     *
     * @param msg Message to encrypt
     * @param ad Associated Data to authenticate
     * @return nonce+encrypted+mac
     * @throws OpenSslError
     * @throws MacVerificationError
     */
    std::string encrypt(std::string_view msg, std::string_view ad = {});

    /**
     * Decrypt a message in the format nonce+encrypted+mac.
     *
     * @param ct Message to decrypt
     * @param ad Associated Data to authenticate
     * @return Decrypted message
     * @throws OpenSslError
     * @throws MacVerificationError
     */
    std::string decrypt(std::string_view ct, std::string_view ad = {});

    /**
     * Encrypt a message with explicit nonce.
     *
     * @param nonce Nonce/IV to use
     * @param[out] ct Ciphertext
     * @param[out] mac MAC tag
     * @param msg Message to encrypt
     * @param ad Associated Data to authenticate
     * @throws OpenSslError
     * @throws MacVerificationError
     */
    virtual void encrypt(std::string_view nonce,
                         gsl::span<char> ct,
                         gsl::span<char> mac,
                         std::string_view msg,
                         std::string_view ad = {}) = 0;

    /**
     * Decrypt a message with explicit nonce.
     *
     * @param nonce Nonce/IV to use
     * @param ct Ciphertext
     * @param mac MAC tag
     * @param[out] msg Decrypted message
     * @param ad Associated Data to authenticate
     * @throws OpenSslError
     * @throws MacVerificationError
     */
    virtual void decrypt(std::string_view nonce,
                         std::string_view ct,
                         std::string_view mac,
                         gsl::span<char> msg,
                         std::string_view ad = {}) = 0;

    virtual std::size_t getNonceSize() const = 0;

    virtual std::size_t getMacSize() const = 0;

    virtual ~SymmetricCipher() = default;

protected:
    SymmetricCipher() = default;
};

} // namespace cb::crypto

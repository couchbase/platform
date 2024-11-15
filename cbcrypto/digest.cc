/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include "platform/string_hex.h"

#include <cbcrypto/digest.h>
#include <fmt/format.h>
#include <folly/portability/Fcntl.h>
#include <folly/portability/Unistd.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <phosphor/phosphor.h>
#include <sodium.h>

#include <memory>
#include <stdexcept>

namespace cb::crypto {
namespace internal {

static std::string HMAC_SHA1(std::string_view key, std::string_view data) {
    std::string ret;
    ret.resize(SHA1_DIGEST_SIZE);
    if (HMAC(EVP_sha1(),
             key.data(),
             key.size(),
             reinterpret_cast<const uint8_t*>(data.data()),
             data.size(),
             reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())),
             nullptr) == nullptr) {
        throw std::runtime_error("cb::crypto::HMAC(SHA1): HMAC failed");
    }
    return ret;
}

static std::string HMAC_SHA256(std::string_view key, std::string_view data) {
    std::string ret;
    ret.resize(SHA256_DIGEST_SIZE);
    if (HMAC(EVP_sha256(),
             key.data(),
             key.size(),
             reinterpret_cast<const uint8_t*>(data.data()),
             data.size(),
             reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())),
             nullptr) == nullptr) {
        throw std::runtime_error("cb::crypto::HMAC(SHA256): HMAC failed");
    }
    return ret;
}

static std::string HMAC_SHA512(std::string_view key, std::string_view data) {
    std::string ret;
    ret.resize(SHA512_DIGEST_SIZE);
    if (HMAC(EVP_sha512(),
             key.data(),
             key.size(),
             reinterpret_cast<const uint8_t*>(data.data()),
             data.size(),
             reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())),
             nullptr) == nullptr) {
        throw std::runtime_error("cb::crypto::HMAC(SHA512): HMAC failed");
    }
    return ret;
}

static std::string PBKDF2_HMAC_SHA1(std::string_view pass,
                                    std::string_view salt,
                                    unsigned int iterationCount) {
    std::string ret;
    ret.resize(SHA1_DIGEST_SIZE);
    auto err = PKCS5_PBKDF2_HMAC(
            pass.data(),
            int(pass.size()),
            reinterpret_cast<const uint8_t*>(salt.data()),
            int(salt.size()),
            iterationCount,
            EVP_sha1(),
            SHA1_DIGEST_SIZE,
            reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())));

    if (err != 1) {
        throw std::runtime_error(
                "cb::crypto::PBKDF2_HMAC(SHA1): PKCS5_PBKDF2_HMAC_SHA1 "
                "failed: " +
                std::to_string(err));
    }

    return ret;
}

static std::string PBKDF2_HMAC_SHA256(std::string_view pass,
                                      std::string_view salt,
                                      unsigned int iterationCount) {
    std::string ret;
    ret.resize(SHA256_DIGEST_SIZE);
    auto err = PKCS5_PBKDF2_HMAC(
            pass.data(),
            int(pass.size()),
            reinterpret_cast<const uint8_t*>(salt.data()),
            int(salt.size()),
            iterationCount,
            EVP_sha256(),
            SHA256_DIGEST_SIZE,
            reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())));
    if (err != 1) {
        throw std::runtime_error(
                "cb::crypto::PBKDF2_HMAC(SHA256): PKCS5_PBKDF2_HMAC failed" +
                std::to_string(err));
    }

    return ret;
}

static std::string PBKDF2_HMAC_SHA512(std::string_view pass,
                                      std::string_view salt,
                                      unsigned int iterationCount) {
    std::string ret;
    ret.resize(SHA512_DIGEST_SIZE);
    auto err = PKCS5_PBKDF2_HMAC(
            pass.data(),
            int(pass.size()),
            reinterpret_cast<const uint8_t*>(salt.data()),
            int(salt.size()),
            iterationCount,
            EVP_sha512(),
            SHA512_DIGEST_SIZE,
            reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())));
    if (err != 1) {
        throw std::runtime_error(
                "cb::crypto::PBKDF2_HMAC(SHA512): PKCS5_PBKDF2_HMAC failed" +
                std::to_string(err));
    }

    return ret;
}

static std::string digest_sha1(std::string_view data) {
    std::string ret;
    ret.resize(SHA1_DIGEST_SIZE);
    SHA1(reinterpret_cast<const uint8_t*>(data.data()),
         data.size(),
         reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())));
    return ret;
}

static std::string digest_sha256(std::string_view data) {
    std::string ret;
    ret.resize(SHA256_DIGEST_SIZE);
    SHA256(reinterpret_cast<const uint8_t*>(data.data()),
           data.size(),
           reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())));
    return ret;
}

static std::string digest_sha512(std::string_view data) {
    std::string ret;
    ret.resize(SHA512_DIGEST_SIZE);
    SHA512(reinterpret_cast<const uint8_t*>(data.data()),
           data.size(),
           reinterpret_cast<uint8_t*>(const_cast<char*>(ret.data())));
    return ret;
}
} // namespace internal

std::string HMAC(const Algorithm algorithm,
                 std::string_view key,
                 std::string_view data) {
    TRACE_EVENT1("cbcrypto", "HMAC", "algorithm", int(algorithm));
    switch (algorithm) {
    case Algorithm::SHA1:
        return internal::HMAC_SHA1(key, data);
    case Algorithm::SHA256:
        return internal::HMAC_SHA256(key, data);
    case Algorithm::SHA512:
        return internal::HMAC_SHA512(key, data);
    case Algorithm::Argon2id13:
    case Algorithm::DeprecatedPlain:
        throw std::invalid_argument(
                "cb::crypto::HMAC(): Can't be called with Argon2id13");
    }

    throw std::invalid_argument("cb::crypto::HMAC: Unknown Algorithm: " +
                                std::to_string((int)algorithm));
}

std::string PBKDF2_HMAC(const Algorithm algorithm,
                        std::string_view pass,
                        std::string_view salt,
                        unsigned int iterationCount) {
    if (iterationCount == 0) {
        throw std::invalid_argument(
                "cb::crypto::PBKDF2_HMAC: Iteration count can't be 0");
    }
    TRACE_EVENT2("cbcrypto",
                 "PBKDF2_HMAC",
                 "algorithm",
                 int(algorithm),
                 "iteration",
                 iterationCount);
    switch (algorithm) {
    case Algorithm::SHA1:
        return internal::PBKDF2_HMAC_SHA1(pass, salt, iterationCount);
    case Algorithm::SHA256:
        return internal::PBKDF2_HMAC_SHA256(pass, salt, iterationCount);
    case Algorithm::SHA512:
        return internal::PBKDF2_HMAC_SHA512(pass, salt, iterationCount);
    case Algorithm::DeprecatedPlain:
    case Algorithm::Argon2id13:
        throw std::invalid_argument(
                "cb::crypto::PBKDF2_HMAC(): Can't be called with Argon2id13");
    }

    throw std::invalid_argument("cb::crypto::PBKDF2_HMAC: Unknown Algorithm: " +
                                std::to_string((int)algorithm));
}

static std::string pbkdf2_hmac(const Algorithm algorithm,
                               std::string_view pass,
                               std::string_view salt,
                               const nlohmann::json& properties) {
    return PBKDF2_HMAC(
            algorithm, pass, salt, properties.value("iterations", 0));
}

static std::string argon2id13_pwhash(std::string_view password,
                                     std::string_view salt,
                                     uint64_t opslimit,
                                     size_t memlimit) {
    if (!opslimit || !memlimit) {
        throw std::invalid_argument(
                "argon2id13_pwhash(): time or memory can't be 0");
    }

    std::string generated;
    generated.resize(Argon2id13DigestSize);
    if (crypto_pwhash(reinterpret_cast<unsigned char*>(generated.data()),
                      generated.size(),
                      password.data(),
                      password.size(),
                      reinterpret_cast<const unsigned char*>(salt.data()),
                      opslimit,
                      memlimit,
                      crypto_pwhash_argon2id_alg_argon2id13()) == -1) {
        // According to https://doc.libsodium.org/password_hashing/default_phf
        // it states:
        // The function returns 0 on success and -1 if the computation didn't
        // complete, usually because the operating system refused to allocate
        // the amount of requested memory.
        throw std::bad_alloc();
    };
    return generated;
}

std::string pwhash(Algorithm algorithm,
                   std::string_view password,
                   std::string_view salt,
                   const nlohmann::json& properties) {
    switch (algorithm) {
    case Algorithm::SHA1:
    case Algorithm::SHA256:
    case Algorithm::SHA512:
        return pbkdf2_hmac(algorithm, password, salt, properties);
    case Algorithm::Argon2id13:
        return argon2id13_pwhash(password,
                                 salt,
                                 properties.value("time", uint64_t(0)),
                                 properties.value("memory", std::size_t(0)));
    case Algorithm::DeprecatedPlain:
        return HMAC(Algorithm::SHA1, salt, password);
    }
    throw std::invalid_argument("pwhash(): Unknown algorithm");
}

std::string digest(const Algorithm algorithm, std::string_view data) {
    TRACE_EVENT1("cbcrypto", "digest", "algorithm", int(algorithm));
    switch (algorithm) {
    case Algorithm::SHA1:
        return internal::digest_sha1(data);
    case Algorithm::SHA256:
        return internal::digest_sha256(data);
    case Algorithm::SHA512:
        return internal::digest_sha512(data);
    case Algorithm::DeprecatedPlain:
    case Algorithm::Argon2id13:
        throw std::invalid_argument(
                "cb::crypto::digest: can't be called with Argon2id13");
    }

    throw std::invalid_argument("cb::crypto::digest: Unknown Algorithm" +
                                std::to_string((int)algorithm));
}

std::string sha512sum(const std::filesystem::path& path,
                      std::size_t size,
                      const std::size_t chunksize) {
    // Create some custom deleters to allow for using std::unique_ptr
    // to clean up resource usage in error paths
    struct EVP_MD_Deleter {
        void operator()(EVP_MD* md) {
            EVP_MD_free(md);
        }
    };

    struct EVP_MD_CTX_Deleter {
        void operator()(EVP_MD_CTX* ctx) {
            EVP_MD_CTX_free(ctx);
        }
    };

    struct FILE_Deleter {
        void operator()(FILE* fp) {
            fclose(fp);
        }
    };

    std::unique_ptr<EVP_MD, EVP_MD_Deleter> md(
            EVP_MD_fetch(nullptr, "SHA512", nullptr));
    std::unique_ptr<EVP_MD_CTX, EVP_MD_CTX_Deleter> ctx(EVP_MD_CTX_create());

    if (!md) {
        throw std::runtime_error(
                "cb::crypto::digest_sha512: EVP_MD_fetch() failed");
    }
    if (!ctx) {
        throw std::runtime_error(
                "cb::crypto::digest_sha512: EVP_MD_CTX_create() failed");
    }

    if (EVP_DigestInit_ex(ctx.get(), md.get(), nullptr) == 0) {
        throw std::runtime_error(
                "cb::crypto::digest_sha512: EVP_DigestInit_ex() failed");
    }

    std::vector<unsigned char> buffer(chunksize);
    std::size_t offset = 0;
    if (size == 0) {
        size = file_size(path);
    }

    std::unique_ptr<FILE, FILE_Deleter> fp(fopen(path.string().c_str(), "rb"));
    if (!fp) {
        throw std::system_error(
                errno, std::system_category(), "Failed to open file");
    }

#ifdef __linux__
    // Give the kernel a hint that we'll read the data
    // sequentially and won't need it more than once
    (void)posix_fadvise(
            fileno(fp.get()), 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_NOREUSE);
#endif

    while (offset < size) {
        auto chunk = std::min(size - offset, buffer.size());
        auto nr = fread(buffer.data(), chunk, 1, fp.get());
        if (nr != 1) {
            if (feof(fp.get())) {
                throw std::runtime_error(fmt::format(
                        "Read error: End of file (still {} bytes left to read)",
                        size - offset));
            }
            if (ferror(fp.get())) {
                throw std::system_error(
                        errno,
                        std::system_category(),
                        fmt::format("Read error at offset:{}", offset));
            }
            throw std::runtime_error(
                    "Read error: Not EOF and ferror did not indicate an error");
        }
        offset += chunk;
        if (EVP_DigestUpdate(ctx.get(), buffer.data(), chunk) == 0) {
            throw std::runtime_error(
                    "cb::crypto::digest_sha512: EVP_DigestUpdate() failed");
        }
    }
    fp.reset();

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest;
    unsigned int nbytes = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &nbytes) == 0) {
        throw std::runtime_error(
                "cb::crypto::digest_sha512: EVP_DigestFinal_ex() failed");
    }

    return hex_encode({digest.data(), nbytes});
}

} // namespace cb::crypto
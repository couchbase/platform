/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>

namespace cb::crypto {
enum class Algorithm { SHA1, SHA256, SHA512, Argon2id13, DeprecatedPlain };

constexpr int SHA1_DIGEST_SIZE = 20;
constexpr int SHA256_DIGEST_SIZE = 32;
constexpr int SHA512_DIGEST_SIZE = 64;
constexpr std::size_t Argon2id13DigestSize = 32;

/**
 * Generate a HMAC digest of the key and data by using the given
 * algorithm
 *
 * @throws std::invalid_argument - unsupported algorithm
 *         std::runtime_error - Failures generating the HMAC
 */
std::string HMAC(Algorithm algorithm,
                 std::string_view key,
                 std::string_view data);

/**
 * Generate a PBKDF2_HMAC digest of the key and data by using the given
 * algorithm
 *
 * @throws std::invalid_argument - unsupported algorithm
 *         std::runtime_error - Failures generating the HMAC
 */
std::string PBKDF2_HMAC(Algorithm algorithm,
                        std::string_view pass,
                        std::string_view salt,
                        unsigned int iterationCount);

/**
 * Generate a digest by using the requested algorithm
 */
std::string digest(Algorithm algorithm, std::string_view data);

/**
 * Generate a password hash with the given algorithm
 *
 * @param algorithm The algorithm to use
 * @param password The password to hash
 * @param salt  The salt to use
 * @param properties Additional properties on a per-algorithm base
 * @return The password hash
 */
std::string pwhash(Algorithm algorithm,
                   std::string_view password,
                   std::string_view salt,
                   const nlohmann::json& properties = {});

/**
 * Create a SHA512 sum for a file and return the sum as a printable hex string
 * which may be compared with the hex string printed with the command line
 * utility sha512sum.
 *
 * Note: The output from this function only contains the sum!
 * Note: The method may throw additional exceptions from the ones listed
 *       below. These are the ones explicitly thrown from the method
 *
 * @param path The file to calculate the hash for
 * @param size The number of bytes in the file to include in the sum. If
 *             set to 0 it'll read the entire file
 * @param chunksize The number of bytes to try to read from the file in each
 *               chunk
 * @return The textual SHA512 sum
 * @throws std::bad_alloc for memory allocation failures
 * @throws std::runtime_error for errors related to OpenSSL (or if
 *                            EOF is reached before size bytes is read)
 * @throws std::system_error for errors related to file IO
 */
std::string sha512sum(const std::filesystem::path& path,
                      std::size_t size = 0,
                      std::size_t chunksize = 1024 * 1024);

} // namespace cb::crypto

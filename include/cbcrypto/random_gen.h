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

/**
 * Cryptographically secure random bit generator
 */
class RandomBitGenerator {
public:
    /**
     * Instantiates a RandomBitGenerator.
     *
     * @param properties Properties to pass to OpenSSL (e.g. provider)
     * @throws OpenSslError
     */
    static std::unique_ptr<RandomBitGenerator> create(
            const char* properties = nullptr);

    /**
     * Populate `buf` with random bytes.
     *
     * @throws OpenSslError
     */
    virtual void generate(gsl::span<char> buf) = 0;

    virtual ~RandomBitGenerator() = default;
};

} // namespace cb::crypto

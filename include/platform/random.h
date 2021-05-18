/*
 *     Copyright 2014-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace cb {
class RandomGeneratorProvider;

/**
 * The RandomGenerator use windows crypto framework on windows and
 * /dev/urandom on the other platforms in order to get random data.
 */
class RandomGenerator {
public:
    RandomGenerator();

    uint64_t next();

    bool getBytes(void* dest, size_t size);

private:
    /// Return the singleton instance of RandomGeneratorProvider.
    static RandomGeneratorProvider& getInstance();
};
} // namespace cb

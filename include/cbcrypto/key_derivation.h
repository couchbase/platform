/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <string_view>

namespace cb::crypto {

enum class KeyDerivationFunction {
    HMAC_SHA256_COUNTER,
    HMAC_SHA256_COUNTER_WITHOUT_L
};

/**
 * Derive a new key from a key derivation key
 *
 * @param derivedSize Size of derived key in bytes
 * @param kdk Key Derivation Key
 * @param label Purpose of derived key
 * @param context Contextual info, e.g. component name, identifier, randomness
 * @param kdf Key Derivation Function variant
 * @return Derived key
 */
std::string deriveKey(
        std::size_t derivedSize,
        std::string_view kdk,
        std::string_view label,
        std::string_view context,
        KeyDerivationFunction kdf = KeyDerivationFunction::HMAC_SHA256_COUNTER);

} // namespace cb::crypto

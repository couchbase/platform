/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <cbcrypto/common.h>

#include <atomic>
#include <functional>

namespace cb::logger {

/**
 * Access to the log encryption key material, injected into initialize() by the
 * embedder
 *
 * The file sink writes plain or encrypted files depending on what getKey()
 * returns, if empty key then unencrypted files are written
 * @param getKey function returning the current key to use for encrypting log
 * files
 */
struct FileSinkEncryptionConfig {
    std::function<cb::crypto::SharedKeyDerivationKey()> getKey;

    std::atomic_uint64_t* configVersion{nullptr};
};

} // namespace cb::logger

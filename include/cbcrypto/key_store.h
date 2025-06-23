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
#include <nlohmann/json_fwd.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace cb ::crypto {
/**
 * The KeyStore is a class which holds multiple keys and allow for
 * looking up keys with an ID. The keystore includes an active key
 */
class KeyStore {
public:
    /// Get the current active key (note: it may be empty)
    [[nodiscard]] SharedEncryptionKey getActiveKey() const {
        return active;
    }

    /// Look up a given key by its id (or return {} if the id is unknown)
    [[nodiscard]] SharedEncryptionKey lookup(std::string_view id) const;

    /// set the provided key as active (and add it to the list of keys_
    void setActiveKey(SharedEncryptionKey key);

    /// Add a key to the list of keys (but do not mark it as active)
    void add(SharedEncryptionKey key);

    /// Iterate over all known keys and call the provided callback
    void iterateKeys(
            const std::function<void(SharedEncryptionKey)>& callback) const;

protected:
    std::vector<SharedEncryptionKey> keys;
    SharedEncryptionKey active;
};

[[nodiscard]] std::string format_as(const KeyStore& ks);
void to_json(nlohmann::json& json, const KeyStore& ks);
void from_json(const nlohmann::json& json, KeyStore& ks);

/// Convert a keystore to a JSON object we may put in our logs
/// (strip off the actual keys etc)
nlohmann::json toLoggableJson(const KeyStore& keystore);

} // namespace cb::crypto

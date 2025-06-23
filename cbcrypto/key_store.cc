/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cbcrypto/common.h>
#include <cbcrypto/key_store.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace cb::crypto {

SharedEncryptionKey KeyStore::lookup(std::string_view id) const {
    for (const auto& k : keys) {
        if (k->id == id) {
            return k;
        }
    }
    return {};
}

void KeyStore::setActiveKey(SharedEncryptionKey key) {
    if (active && key && active->id == key->id) {
        // This represents the same key
        return;
    }
    add(key);
    active = std::move(key);
}

void KeyStore::add(SharedEncryptionKey key) {
    if (!key) {
        return;
    }

    for (const auto& k : keys) {
        if (k->id == key->id) {
            return;
        }
    }

    keys.emplace_back(std::move(key));
}

void KeyStore::iterateKeys(
        const std::function<void(SharedEncryptionKey)>& callback) const {
    for (auto& key : keys) {
        callback(key);
    }
}

[[nodiscard]] std::string format_as(const KeyStore& ks) {
    return nlohmann::json(ks).dump();
}

void to_json(nlohmann::json& json, const KeyStore& ks) {
    json = nlohmann::json::object();
    auto active = ks.getActiveKey();
    if (active) {
        json["active"] = active->id;
    }

    nlohmann::json keys = nlohmann::json::array();
    ks.iterateKeys([&keys](auto k) { keys.emplace_back(*k); });

    if (!keys.empty()) {
        json["keys"] = std::move(keys);
    }
}

void from_json(const nlohmann::json& json, KeyStore& ks) {
    ks = {};

    const std::string value = json.dump();

    if (!json.is_object()) {
        throw std::runtime_error(
                "from_json(KeyStore): Provided json should be an "
                "object");
    }

    if (json.contains("keys")) {
        for (const auto& obj : json["keys"].get<nlohmann::json::array_t>()) {
            auto object = std::make_shared<crypto::DataEncryptionKey>();
            *object = obj;
            ks.add(object);
        }
    }

    const auto active = json.value("active", std::string{});
    if (!active.empty()) {
        auto object = ks.lookup(active);
        if (!object) {
            throw std::runtime_error(fmt::format(
                    R"(from_json(KeyStore): The active key "{}" does not exists)",
                    active));
        }
        ks.setActiveKey(object);
    } else {
        ks.setActiveKey({});
    }
}

nlohmann::json toLoggableJson(const cb::crypto::KeyStore& keystore) {
    nlohmann::json ids = nlohmann::json::array();
    keystore.iterateKeys([&ids](auto key) { ids.emplace_back(key->getId()); });
    nlohmann::json entry;
    entry["keys"] = std::move(ids);
    if (keystore.getActiveKey()) {
        entry["active"] = keystore.getActiveKey()->getId();
    }
    return entry;
}

} // namespace cb::crypto

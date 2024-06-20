/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "platform/base64.h"

#include <cbcrypto/common.h>
#include <cbcrypto/key_store.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>

using namespace cb::crypto;
using namespace cb::base64;
using namespace std::string_view_literals;

const nlohmann::json BluePrint = R"(
{
  "active": "489cf03d-07f1-4e4c-be6f-01f227757937",
  "keys": [
    {
      "cipher": "AES-256-GCM",
      "id": "489cf03d-07f1-4e4c-be6f-01f227757937",
      "key": "cXOdH9oGE834Y2rWA+FSdXXi5CN3mLJ+Z+C0VpWbOdA="
    },
    {
      "cipher": "AES-256-GCM",
      "id": "c7e26d06-88ed-43bc-9f66-87b60c037211",
      "key": "ZdA1gPe3Z4RRfC+r4xjBBCKYtYJ9dNOOLxNEC0zjKVY="
    }
  ]
}
)"_json;

class KeyStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        ks = BluePrint;
    }

    size_t countKeys() {
        size_t num = 0;
        ks.iterateKeys([&num](auto k) { ++num; });
        return num;
    }

    KeyStore ks;
};

TEST_F(KeyStoreTest, GetActiveKey) {
    auto active = ks.getActiveKey();
    ASSERT_TRUE(active);
    EXPECT_EQ("489cf03d-07f1-4e4c-be6f-01f227757937"sv, active->id);
    EXPECT_EQ("AES-256-GCM"sv, active->cipher);
    EXPECT_EQ("cXOdH9oGE834Y2rWA+FSdXXi5CN3mLJ+Z+C0VpWbOdA="sv,
              encode(active->key));
}

TEST_F(KeyStoreTest, SetActiveKey) {
    EXPECT_EQ(2, countKeys());
    ks.setActiveKey({});
    EXPECT_FALSE(ks.getActiveKey());
    EXPECT_EQ(2, countKeys());
    auto next = DataEncryptionKey::generate();
    ks.setActiveKey(next);
    EXPECT_EQ(3, countKeys());

    auto active = ks.getActiveKey();
    EXPECT_EQ(next->id, active->id);

    // If we try to set it again "nothing" should happen
    ks.setActiveKey(next);
    EXPECT_EQ(3, countKeys());
}

TEST_F(KeyStoreTest, LookupNonExistingId) {
    EXPECT_FALSE(ks.lookup("non-existing-id"));
}

TEST_F(KeyStoreTest, LookupKey) {
    auto second = ks.lookup("c7e26d06-88ed-43bc-9f66-87b60c037211");
    ASSERT_TRUE(second);
    EXPECT_EQ("c7e26d06-88ed-43bc-9f66-87b60c037211"sv, second->id);
    EXPECT_EQ("AES-256-GCM"sv, second->cipher);
    EXPECT_EQ("ZdA1gPe3Z4RRfC+r4xjBBCKYtYJ9dNOOLxNEC0zjKVY="sv,
              encode(second->key));
}

TEST_F(KeyStoreTest, iterateKeys) {
    // verify those keys are the only one
    bool foundActive = false;
    bool foundSecond = false;
    ks.iterateKeys([&foundActive, &foundSecond](auto key) {
        ASSERT_TRUE(key);
        if (key->id == "489cf03d-07f1-4e4c-be6f-01f227757937") {
            EXPECT_FALSE(foundActive) << "Active key already be reported";
            foundActive = true;
            EXPECT_EQ("AES-256-GCM"sv, key->cipher);
            EXPECT_EQ("cXOdH9oGE834Y2rWA+FSdXXi5CN3mLJ+Z+C0VpWbOdA="sv,
                      encode(key->key));
        } else if (key->id == "c7e26d06-88ed-43bc-9f66-87b60c037211") {
            EXPECT_FALSE(foundSecond) << "Second key already reported";
            foundSecond = true;
            EXPECT_EQ("AES-256-GCM"sv, key->cipher);
            EXPECT_EQ("ZdA1gPe3Z4RRfC+r4xjBBCKYtYJ9dNOOLxNEC0zjKVY="sv,
                      encode(key->key));
        } else {
            FAIL() << "Unexpected key: " << key->id;
        }
    });
    EXPECT_TRUE(foundActive);
    EXPECT_TRUE(foundSecond);
}

TEST_F(KeyStoreTest, ToJson) {
    nlohmann::json converted = ks;
    EXPECT_EQ(converted, BluePrint);
}

TEST_F(KeyStoreTest, Add) {
    EXPECT_EQ(2, countKeys());

    auto active = ks.getActiveKey();
    auto next = DataEncryptionKey::generate();
    ks.add(next);
    EXPECT_EQ(3, countKeys());
    bool found = false;
    ks.iterateKeys([&found, next](auto key) {
        if (next == key) {
            EXPECT_FALSE(found);
            found = true;
        }
    });
    EXPECT_TRUE(found);
    // ensure it didn't change the active key
    EXPECT_EQ(active, ks.getActiveKey());
}

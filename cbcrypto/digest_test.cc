/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cbcrypto/digest.h>
#include <folly/portability/GTest.h>

TEST(Digest, SHA256HexEncoded) {
    EXPECT_EQ(
            "d14e761bce43dd11cde7c567d6ec65595099f2512b23f44eb6f3a2278d5761c1",
            cb::crypto::digest(cb::crypto::Algorithm::SHA256,
                               "this is the input data",
                               cb::crypto::HexString::Yes));
}

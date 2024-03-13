/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "platform/sysinfo.h"
#include <folly/portability/GTest.h>

#include <platform/unshared.h>

TEST(Unshared, Init) {
    cb::MonoUnshared<> counter;

    counter.setCoreThreshold(1);

    EXPECT_EQ(0, counter.getEstimate());
    EXPECT_EQ(0, counter.getEstimateSum());
    EXPECT_EQ(0, counter.getPrecise());
    EXPECT_EQ(0, counter.getPreciseSum());
    EXPECT_EQ(1, counter.getCoreThreshold());
    EXPECT_EQ(1 * cb::get_cpu_count(), counter.getMaximumDrift());
}

TEST(Unshared, Reset) {
    cb::MonoUnshared<> counter;

    counter.setCoreThreshold(1);
    counter.add(10);
    counter.reset();

    EXPECT_EQ(0, counter.getEstimate());
    EXPECT_EQ(0, counter.getEstimateSum());
    EXPECT_EQ(0, counter.getPrecise());
    EXPECT_EQ(0, counter.getPreciseSum());
    EXPECT_EQ(1, counter.getCoreThreshold());
}

TEST(Unshared, Add) {
    cb::MonoUnshared<> counter;

    counter.setCoreThreshold(1);
    counter.add(10);

    EXPECT_EQ(10, counter.getEstimate());
    EXPECT_EQ(10, counter.getPrecise());
    EXPECT_EQ(10, counter.getEstimateSum());
    EXPECT_EQ(10, counter.getPreciseSum());
}

TEST(Unshared, AddNegative) {
    cb::MonoUnshared<> counter;

    counter.setCoreThreshold(1);
    counter.add(-10);

    EXPECT_EQ(0, counter.getEstimate());
    EXPECT_EQ(0, counter.getPrecise());
    EXPECT_EQ(0, counter.getEstimateSum());
    EXPECT_EQ(0, counter.getPreciseSum());
}

// Pushing the counters into the negative has no observable effect, as the
// values are capped to 0.
TEST(Unshared, Sub) {
    cb::MonoUnshared<> counter;

    counter.setCoreThreshold(1);
    counter.sub(10);

    EXPECT_EQ(0, counter.getEstimate());
    EXPECT_EQ(0, counter.getPrecise());
    EXPECT_EQ(0, counter.getEstimateSum());
    EXPECT_EQ(0, counter.getPreciseSum());
}

TEST(Unshared, SubNegative) {
    cb::MonoUnshared<> counter;

    counter.setCoreThreshold(1);
    // Subtracting negative increases the total.
    counter.sub(-10);

    EXPECT_EQ(10, counter.getEstimate());
    EXPECT_EQ(10, counter.getPrecise());
    EXPECT_EQ(10, counter.getEstimateSum());
    EXPECT_EQ(10, counter.getPreciseSum());
}

// Check that the thresholds are respected.
TEST(Unshared, Thresholds) {
    cb::MonoUnshared<> counter;

    counter.setCoreThreshold(1);
    counter.add(5);
    // Set threshold to 5 and add 5. We shouldn't update the counter until we've
    // drifted by 6 or more.
    counter.setCoreThreshold(5);
    counter.add(5);

    EXPECT_EQ(5, counter.getEstimateSum());
    EXPECT_EQ(10, counter.getPreciseSum());
}

enum class TestIndex {
    First = 0,
    Second = 1,
    Count = 2,
};

// Check that we can use multiple counters and their sum is correct.
TEST(Unshared, Multiple) {
    cb::Unshared<TestIndex> counter;
    counter.setCoreThreshold(10);
    counter.add(5, TestIndex::First);
    counter.add(15, TestIndex::Second);

    EXPECT_EQ(0, counter.getEstimate(TestIndex::First));
    EXPECT_EQ(15, counter.getEstimate(TestIndex::Second));

    EXPECT_EQ(15, counter.getEstimateSum());
    EXPECT_EQ(20, counter.getPreciseSum());

    EXPECT_EQ(5, counter.getEstimate(TestIndex::First));
    EXPECT_EQ(15, counter.getEstimate(TestIndex::Second));

    EXPECT_EQ(5, counter.getPrecise(TestIndex::First));
    EXPECT_EQ(15, counter.getPrecise(TestIndex::Second));
}

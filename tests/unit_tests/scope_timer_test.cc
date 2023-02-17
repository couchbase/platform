/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/scope_timer.h>

#include <folly/portability/GTest.h>

#include <vector>

class MockTimer {
public:
    // Duration since unix epoch representation stored in the start / stop times
    // vectors. We use this and not the higher-level steady_clock::duration or
    // time_point for ease of use with GTest EXPECT_xxx() macros - it can
    // print the values of the underlying representation, but not necessarily
    // duration or time_point.
    using SinceEpoch = std::chrono::steady_clock::duration::rep;

    // Ordered collection of start / stop times recorded by the MockTimer.
    using TimeVector = std::vector<SinceEpoch>;

    MockTimer(TimeVector& startTimes,
              TimeVector& stopTimes)
        : startTimes(startTimes), stopTimes(stopTimes) {
    }

    ~MockTimer() {
        EXPECT_TRUE(startCalled);
    }

    void start(std::chrono::steady_clock::time_point time) {
        startTimes.emplace_back(time.time_since_epoch().count());
        startCalled = true;
    }

    void stop(std::chrono::steady_clock::time_point time) {
        EXPECT_TRUE(startCalled);
        stopTimes.emplace_back(time.time_since_epoch().count());
    }

    bool isEnabled() const {
        return true;
    }

    bool startCalled = false;
    TimeVector& startTimes;
    TimeVector& stopTimes;
};

class ScopeTimerTest : public ::testing::Test {
protected:
    MockTimer::TimeVector start;
    MockTimer::TimeVector stop;
};

TEST_F(ScopeTimerTest, SingleListener) {
    { ScopeTimer1<MockTimer> timer(start, stop); }

    EXPECT_EQ(1u, start.size());
    EXPECT_EQ(1u, stop.size());
    EXPECT_GT(stop[0], start[0]);
}

TEST_F(ScopeTimerTest, TwoListeners) {
    {
        ScopeTimer2<MockTimer, MockTimer> timer(
                std::forward_as_tuple(start, stop),
                std::forward_as_tuple(start, stop));
    }

    EXPECT_EQ(2u, start.size());
    EXPECT_EQ(2u, stop.size());
    EXPECT_GT(stop[0], start[0]);
    EXPECT_GT(stop[1], start[1]);
    EXPECT_EQ(start[0], start[1])
            << "ScopeTimer listeners did not receive the same start time.";
    EXPECT_EQ(stop[0], stop[1])
            << "ScopeTimer listeners did not receive the same stop time.";
}

/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <platform/scope_timer.h>

#include <folly/portability/GTest.h>

#include <vector>

class MockTimer {
public:
    MockTimer(std::vector<std::chrono::steady_clock::time_point>& startTimes,
              std::vector<std::chrono::steady_clock::time_point>& stopTimes)
        : startTimes(startTimes), stopTimes(stopTimes) {
    }

    void start(std::chrono::steady_clock::time_point time) {
        startTimes.emplace_back(time);
        startCalled = true;
    }

    void stop(std::chrono::steady_clock::time_point time) {
        EXPECT_TRUE(startCalled);
        stopTimes.emplace_back(time);
    }

    bool isEnabled() const {
        return true;
    }

    bool startCalled = false;
    std::vector<std::chrono::steady_clock::time_point>& startTimes;
    std::vector<std::chrono::steady_clock::time_point>& stopTimes;
};

class ScopeTimerTest : public ::testing::Test {
protected:
    std::vector<std::chrono::steady_clock::time_point> start;
    std::vector<std::chrono::steady_clock::time_point> stop;
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

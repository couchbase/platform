/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "platform/cb_arena_malloc.h"
#include <folly/portability/GTest.h>
#include <platform/guarded.h>

struct TestGuard {
    TestGuard() {
        ++current;
        ++total;
    }

    TestGuard(std::string) {
        ++current;
        ++total;
    }

    TestGuard(const TestGuard&) {
        ++current;
        ++total;
    }

    TestGuard(TestGuard&&) {
        ++current;
        ++total;
    }

    TestGuard& operator=(const TestGuard&) = default;
    TestGuard& operator=(TestGuard&&) = default;

    ~TestGuard() {
        --current;
    }

    static int current;
    static int total;
};

int TestGuard::current{0};
int TestGuard::total{0};

TEST(Guarded, GuardIsEnabled) {
    {
        cb::Guarded<int, TestGuard> num;

        EXPECT_EQ(0, TestGuard::current);
        EXPECT_EQ(1, TestGuard::total);

        int numCopy = num.withGuard([](int n) {
            EXPECT_EQ(1, TestGuard::current);
            EXPECT_EQ(2, TestGuard::total);
            return n;
        });
        (void)numCopy;

        EXPECT_EQ(0, TestGuard::current);
        EXPECT_EQ(2, TestGuard::total);
    }
    // Check that the guard is used at destruction.
    EXPECT_EQ(0, TestGuard::current);
    EXPECT_EQ(3, TestGuard::total);
}

TEST(Guarded, MakeGuarded) {
    struct RequiresTestGuard {
        RequiresTestGuard() {
            EXPECT_EQ(1, TestGuard::current);
        }
        RequiresTestGuard(const RequiresTestGuard&) {
            EXPECT_EQ(1, TestGuard::current);
        }
        RequiresTestGuard(RequiresTestGuard&&) {
            EXPECT_EQ(1, TestGuard::current);
        }
        RequiresTestGuard& operator=(const RequiresTestGuard&) = delete;
        RequiresTestGuard& operator=(RequiresTestGuard&&) = delete;
        ~RequiresTestGuard() {
            EXPECT_EQ(1, TestGuard::current);
        }
    };

    cb::Guarded<RequiresTestGuard, TestGuard> test =
            cb::makeGuarded<TestGuard>([]() { return RequiresTestGuard{}; });
}

TEST(Guarded, PiecewiseConstruct) {
    cb::Guarded<int, TestGuard, std::string> num{
            std::piecewise_construct, std::make_tuple(1), std::make_tuple("")};
    EXPECT_EQ(1, num.getUnsafe());
}

TEST(Guarded, Assignment) {
    cb::Guarded<int, TestGuard, std::string> num{
            std::piecewise_construct, std::make_tuple(1), std::make_tuple("")};
    EXPECT_EQ(1, num.getUnsafe());
    num = 2;
    EXPECT_EQ(2, num.getUnsafe());
}

TEST(Guarded, NoArenaGuard) {
    cb::Guarded<std::unique_ptr<int>, cb::NoArenaGuard> ptr{nullptr};
}

TEST(Guarded, ArenaGuard) {
    cb::Guarded<int, cb::ArenaMallocGuard, cb::ArenaMallocClient> ptr{
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(cb::ArenaMallocClient{})};
    EXPECT_EQ(0, ptr.getUnsafe());
}

TEST(Guarded, MakeGuardedNoArena) {
    auto test = cb::makeGuarded<cb::NoArenaGuard>(
            []() { return std::make_unique<int>(10); });
    auto value = test.withGuard([](auto& ptr) { return *ptr; });
    EXPECT_EQ(10, value);
}

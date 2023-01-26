/*
 *    Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "cgroup_private.h"

#include <cgroup/cgroup.h>
#include <folly/portability/GTest.h>
#include <platform/dirutils.h>
#include <filesystem>
#include <fstream>

using namespace cb::cgroup;

class V1 : public ::testing::Test {
protected:
    void SetUp() override {
        test_directory = std::filesystem::path(SOURCE_ROOT) / "test" / "v1";
        instance = cb::cgroup::priv::make_control_group(
                test_directory.generic_string(), pid_t{1});
    }

    std::filesystem::path test_directory;
    std::unique_ptr<cb::cgroup::ControlGroup> instance;
};

class V2 : public ::testing::Test {
protected:
    void SetUp() override {
        test_directory = std::filesystem::path(SOURCE_ROOT) / "test" / "v2";
        instance = cb::cgroup::priv::make_control_group(
                test_directory.generic_string(), pid_t{2});
    }

    std::filesystem::path test_directory;
    std::unique_ptr<cb::cgroup::ControlGroup> instance;
};

enum class Version { V1 = 1, V2 };

/// We map to CGroup V1 if we fail to find the pid in any cgroup2
/// Verify that we behave correctly if the process isn't part of any
/// cgroup.procs file
class NoCgroupFound : public ::testing::TestWithParam<Version> {
protected:
    void SetUp() override {
        switch (GetParam()) {
        case Version::V1:
            test_directory = std::filesystem::path(SOURCE_ROOT) / "test" / "v1";
            break;
        case Version::V2:
            test_directory = std::filesystem::path(SOURCE_ROOT) / "test" / "v2";
            break;
        }
        instance = cb::cgroup::priv::make_control_group(
                test_directory.generic_string(), pid_t{3});
    }

    std::filesystem::path test_directory;
    std::unique_ptr<cb::cgroup::ControlGroup> instance;
};

TEST_F(V1, Version) {
    EXPECT_EQ(ControlGroup::Version::V1, instance->get_version());
}

TEST_F(V1, TestCpuQuota) {
    EXPECT_EQ(250, instance->get_available_cpu());
}

TEST_F(V1, TestMaxMemory) {
    EXPECT_EQ(17179869184, instance->get_max_memory());
}

TEST_F(V1, TestCurrentMemory) {
    EXPECT_EQ(6852075520, instance->get_current_memory());
}

TEST_F(V1, TestCpuStat) {
    auto cpu = instance->get_cpu_stats();
    EXPECT_EQ(205950000, cpu.system.count());
    EXPECT_EQ(3168700000, cpu.user.count());
    EXPECT_EQ(3321492315, cpu.usage.count());
    EXPECT_EQ(0, cpu.burst.count());
    EXPECT_EQ(0, cpu.nr_bursts);
    EXPECT_EQ(6807531343, cpu.throttled.count());
    EXPECT_EQ(12651, cpu.nr_throttled);
    EXPECT_EQ(13498, cpu.nr_periods);
}

TEST_F(V2, Version) {
    EXPECT_EQ(ControlGroup::Version::V2, instance->get_version());
}

TEST_F(V2, TestCpuQuota) {
    EXPECT_EQ(250, instance->get_available_cpu());
}

TEST_F(V2, TestMaxMemory) {
    EXPECT_EQ(8589934592, instance->get_max_memory());
}

TEST_F(V2, TestCurrentMemory) {
    EXPECT_EQ(2766684160, instance->get_current_memory());
}

TEST_F(V2, TestCpuStat) {
    auto cpu = instance->get_cpu_stats();
    EXPECT_EQ(42293504, cpu.system.count());
    EXPECT_EQ(486652575, cpu.user.count());
    EXPECT_EQ(528946079, cpu.usage.count());
    EXPECT_EQ(2261444, cpu.throttled.count());
    EXPECT_EQ(222, cpu.nr_throttled);
    EXPECT_EQ(2914, cpu.nr_periods);
    EXPECT_EQ(0, cpu.nr_bursts);
    EXPECT_EQ(0, cpu.burst.count());
}

TEST_P(NoCgroupFound, TestCpuQuota) {
    EXPECT_LE(100, instance->get_available_cpu());
}

TEST_P(NoCgroupFound, TestMaxMemory) {
    EXPECT_EQ(0, instance->get_max_memory());
}

TEST_P(NoCgroupFound, TestCurrentMemory) {
    EXPECT_EQ(0, instance->get_current_memory());
}

TEST_P(NoCgroupFound, TestCpuStat) {
    auto cpu = instance->get_cpu_stats();
    EXPECT_EQ(0, cpu.system.count());
    EXPECT_EQ(0, cpu.user.count());
    EXPECT_EQ(0, cpu.usage.count());
    EXPECT_EQ(0, cpu.burst.count());
    EXPECT_EQ(0, cpu.nr_bursts);
    EXPECT_EQ(0, cpu.throttled.count());
    EXPECT_EQ(0, cpu.nr_throttled);
    EXPECT_EQ(0, cpu.nr_periods);
}

INSTANTIATE_TEST_SUITE_P(CGroupsTest,
                         NoCgroupFound,
                         ::testing::Values(Version::V1, Version::V2),
                         [](const auto& info) {
                             return "V" + std::to_string(int(info.param));
                         });
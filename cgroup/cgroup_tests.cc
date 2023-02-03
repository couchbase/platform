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
#include <filesystem>

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

TEST_F(V1, TestCurrentCache) {
    EXPECT_EQ(5943459840, instance->get_current_cache_memory());
}

TEST_F(V1, TestMemInfo) {
    auto meminfo = instance->get_mem_info();
    EXPECT_EQ(17179869184, meminfo.max);
    EXPECT_EQ(6852075520, meminfo.current);
    EXPECT_EQ(5943459840, meminfo.cache);
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

TEST_F(V1, TestPressureCpu) {
    EXPECT_FALSE(instance->get_pressure_data(PressureType::Cpu));
}

TEST_F(V1, TestPressureIo) {
    EXPECT_FALSE(instance->get_pressure_data(PressureType::Io));
}

TEST_F(V1, TestPressureMemory) {
    EXPECT_FALSE(instance->get_pressure_data(PressureType::Memory));
}

TEST_F(V1, TestSystemPressureCpu) {
    // some avg10=78.29 avg60=75.76 avg300=66.71 total=733785593
    // full avg10=0.00 avg60=0.00 avg300=0.00 total=0
    auto pressure = instance->get_system_pressure_data(PressureType::Cpu);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(78.29f, data.some.avg10);
    EXPECT_EQ(75.76f, data.some.avg60);
    EXPECT_EQ(66.71f, data.some.avg300);
    EXPECT_EQ(733785593ULL, data.some.total_stall_time.count());
    // Full is undefined for CPU at the system level and set to 0
    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.0f, data.full.avg60);
    EXPECT_EQ(0.0f, data.full.avg300);
    EXPECT_EQ(0ULL, data.full.total_stall_time.count());
}

TEST_F(V1, TestSystemPressureIo) {
    // some avg10=0.01 avg60=0.03 avg300=0.00 total=6691960
    // full avg10=0.00 avg60=0.00 avg300=0.00 total=4176792
    auto pressure = instance->get_system_pressure_data(PressureType::Io);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(0.01f, data.some.avg10);
    EXPECT_EQ(0.03f, data.some.avg60);
    EXPECT_EQ(0.0f, data.some.avg300);
    EXPECT_EQ(6691960ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.0f, data.full.avg60);
    EXPECT_EQ(0.0f, data.full.avg300);
    EXPECT_EQ(4176792ULL, data.full.total_stall_time.count());
}

TEST_F(V1, TestSystemPressureMemory) {
    // some avg10=0.00 avg60=0.04 avg300=0.08 total=855273
    // full avg10=0.00 avg60=0.02 avg300=0.04 total=527201
    auto pressure = instance->get_system_pressure_data(PressureType::Memory);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(0.0f, data.some.avg10);
    EXPECT_EQ(0.04f, data.some.avg60);
    EXPECT_EQ(0.08f, data.some.avg300);
    EXPECT_EQ(855273ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.02f, data.full.avg60);
    EXPECT_EQ(0.04f, data.full.avg300);
    EXPECT_EQ(527201ULL, data.full.total_stall_time.count());
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

TEST_F(V2, TestCurrentCache) {
    EXPECT_EQ(590389248, instance->get_current_cache_memory());
}

TEST_F(V2, TestMemInfo) {
    auto meminfo = instance->get_mem_info();
    EXPECT_EQ(8589934592, meminfo.max);
    EXPECT_EQ(2766684160, meminfo.current);
    EXPECT_EQ(590389248, meminfo.cache);
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

TEST_F(V2, TestPressureCpu) {
    // some avg10=65.95 avg60=69.61 avg300=60.79 total=576908731
    // full avg10=32.86 avg60=32.71 avg300=30.53 total=338250181
    auto pressure = instance->get_pressure_data(PressureType::Cpu);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(65.95f, data.some.avg10);
    EXPECT_EQ(69.61f, data.some.avg60);
    EXPECT_EQ(60.79f, data.some.avg300);
    EXPECT_EQ(576908731ULL, data.some.total_stall_time.count());

    EXPECT_EQ(32.86f, data.full.avg10);
    EXPECT_EQ(32.71f, data.full.avg60);
    EXPECT_EQ(30.53f, data.full.avg300);
    EXPECT_EQ(338250181ULL, data.full.total_stall_time.count());
}

TEST_F(V2, TestPressureIo) {
    // some avg10=0.60 avg60=1.30 avg300=0.76 total=4120174
    // full avg10=0.60 avg60=1.28 avg300=0.75 total=4106664
    auto pressure = instance->get_pressure_data(PressureType::Io);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(0.6f, data.some.avg10);
    EXPECT_EQ(1.3f, data.some.avg60);
    EXPECT_EQ(0.76f, data.some.avg300);
    EXPECT_EQ(4120174ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.6f, data.full.avg10);
    EXPECT_EQ(1.28f, data.full.avg60);
    EXPECT_EQ(0.75f, data.full.avg300);
    EXPECT_EQ(4106664ULL, data.full.total_stall_time.count());
}

TEST_F(V2, TestPressureMemory) {
    // some avg10=1.05 avg60=0.29 avg300=0.06 total=446327
    // full avg10=0.36 avg60=0.11 avg300=0.02 total=308567
    auto pressure = instance->get_pressure_data(PressureType::Memory);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(1.05f, data.some.avg10);
    EXPECT_EQ(0.29f, data.some.avg60);
    EXPECT_EQ(0.06f, data.some.avg300);
    EXPECT_EQ(446327ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.36f, data.full.avg10);
    EXPECT_EQ(0.11f, data.full.avg60);
    EXPECT_EQ(0.02f, data.full.avg300);
    EXPECT_EQ(308567ULL, data.full.total_stall_time.count());
}

TEST_F(V2, TestSystemPressureCpu) {
    // some avg10=78.29 avg60=75.76 avg300=66.71 total=733785593
    // full avg10=0.00 avg60=0.00 avg300=0.00 total=0
    auto pressure = instance->get_system_pressure_data(PressureType::Cpu);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(78.29f, data.some.avg10);
    EXPECT_EQ(75.76f, data.some.avg60);
    EXPECT_EQ(66.71f, data.some.avg300);
    EXPECT_EQ(733785593ULL, data.some.total_stall_time.count());
    // Full is undefined for CPU at the system level and set to 0
    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.0f, data.full.avg60);
    EXPECT_EQ(0.0f, data.full.avg300);
    EXPECT_EQ(0ULL, data.full.total_stall_time.count());
}

TEST_F(V2, TestSystemPressureIo) {
    // some avg10=0.01 avg60=0.03 avg300=0.00 total=6691960
    // full avg10=0.00 avg60=0.00 avg300=0.00 total=4176792
    auto pressure = instance->get_system_pressure_data(PressureType::Io);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(0.01f, data.some.avg10);
    EXPECT_EQ(0.03f, data.some.avg60);
    EXPECT_EQ(0.0f, data.some.avg300);
    EXPECT_EQ(6691960ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.0f, data.full.avg60);
    EXPECT_EQ(0.0f, data.full.avg300);
    EXPECT_EQ(4176792ULL, data.full.total_stall_time.count());
}

TEST_F(V2, TestSystemPressureMemory) {
    // some avg10=0.00 avg60=0.04 avg300=0.08 total=855273
    // full avg10=0.00 avg60=0.02 avg300=0.04 total=527201
    auto pressure = instance->get_system_pressure_data(PressureType::Memory);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(0.0f, data.some.avg10);
    EXPECT_EQ(0.04f, data.some.avg60);
    EXPECT_EQ(0.08f, data.some.avg300);
    EXPECT_EQ(855273ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.02f, data.full.avg60);
    EXPECT_EQ(0.04f, data.full.avg300);
    EXPECT_EQ(527201ULL, data.full.total_stall_time.count());
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

TEST_P(NoCgroupFound, TestCurrentCache) {
    EXPECT_EQ(0, instance->get_current_cache_memory());
}

TEST_P(NoCgroupFound, TestMemInfo) {
    auto meminfo = instance->get_mem_info();
    EXPECT_EQ(0, meminfo.max);
    EXPECT_EQ(0, meminfo.current);
    EXPECT_EQ(0, meminfo.cache);
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

TEST_P(NoCgroupFound, TestPressureCpu) {
    EXPECT_FALSE(instance->get_pressure_data(PressureType::Cpu));
}

TEST_P(NoCgroupFound, TestPressureIo) {
    EXPECT_FALSE(instance->get_pressure_data(PressureType::Io));
}

TEST_P(NoCgroupFound, TestPressureMemory) {
    EXPECT_FALSE(instance->get_pressure_data(PressureType::Memory));
}

TEST_P(NoCgroupFound, TestSystemPressureCpu) {
    // some avg10=78.29 avg60=75.76 avg300=66.71 total=733785593
    // full avg10=0.00 avg60=0.00 avg300=0.00 total=0
    auto pressure = instance->get_system_pressure_data(PressureType::Cpu);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(78.29f, data.some.avg10);
    EXPECT_EQ(75.76f, data.some.avg60);
    EXPECT_EQ(66.71f, data.some.avg300);
    EXPECT_EQ(733785593ULL, data.some.total_stall_time.count());
    // Full is undefined for CPU
}

TEST_P(NoCgroupFound, TestSystemPressureIo) {
    // some avg10=0.01 avg60=0.03 avg300=0.00 total=6691960
    // full avg10=0.00 avg60=0.00 avg300=0.00 total=4176792
    auto pressure = instance->get_system_pressure_data(PressureType::Io);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(0.01f, data.some.avg10);
    EXPECT_EQ(0.03f, data.some.avg60);
    EXPECT_EQ(0.0f, data.some.avg300);
    EXPECT_EQ(6691960ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.0f, data.full.avg60);
    EXPECT_EQ(0.0f, data.full.avg300);
    EXPECT_EQ(4176792ULL, data.full.total_stall_time.count());
}

TEST_P(NoCgroupFound, TestSystemPressureMemory) {
    // some avg10=0.00 avg60=0.04 avg300=0.08 total=855273
    // full avg10=0.00 avg60=0.02 avg300=0.04 total=527201
    auto pressure = instance->get_system_pressure_data(PressureType::Memory);
    ASSERT_TRUE(pressure);
    auto data = *pressure;
    EXPECT_EQ(0.0f, data.some.avg10);
    EXPECT_EQ(0.04f, data.some.avg60);
    EXPECT_EQ(0.08f, data.some.avg300);
    EXPECT_EQ(855273ULL, data.some.total_stall_time.count());

    EXPECT_EQ(0.0f, data.full.avg10);
    EXPECT_EQ(0.02f, data.full.avg60);
    EXPECT_EQ(0.04f, data.full.avg300);
    EXPECT_EQ(527201ULL, data.full.total_stall_time.count());
}

INSTANTIATE_TEST_SUITE_P(CGroupsTest,
                         NoCgroupFound,
                         ::testing::Values(Version::V1, Version::V2),
                         [](const auto& info) {
                             return "V" + std::to_string(int(info.param));
                         });
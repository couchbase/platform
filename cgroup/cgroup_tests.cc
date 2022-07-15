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

#include <boost/filesystem.hpp>
#include <cgroup/cgroup.h>
#include <folly/portability/GTest.h>
#include <platform/dirutils.h>

using namespace cb::cgroup;

class MockControlGroup : public ::testing::Test {
public:
    /// Simulate parts of the CGroup directories I see on Ubuntu 20.04
    static void SetUpTestCase() {
        test_directory = absolute(
                boost::filesystem::path(cb::io::mkdtemp("cgroup_v1_test.")));
        cgroup_directory = test_directory / "sys" / "fs" / "cgroup";
        boost::filesystem::create_directories(test_directory / "proc");
        boost::filesystem::create_directories(cgroup_directory);
        boost::filesystem::create_directories(cgroup_directory / "unified");
        boost::filesystem::create_directories(cgroup_directory / "cpu");
        boost::filesystem::create_directories(cgroup_directory / "cpuacct");
        boost::filesystem::create_directories(cgroup_directory / "memory");

        std::ofstream file(
                absolute(test_directory / "proc" / "mounts").generic_string());

        file << "tempfs " << cgroup_directory.generic_string()
             << " tmpfs ro,nosuid,nodev,noexec,mode=755 0 0\n"
             << "cgroup2 " << (cgroup_directory / "unified").generic_string()
             << " cgroup2 rw,nosuid,nodev,noexec,relatime,nsdelegate 0 0\n"
             << "cpu " << (cgroup_directory / "cpu").generic_string()
             << " cgroup rw,nosuid,nodev,noexec,relatime,cpu 0 0\n"
             << "cpuacct " << (cgroup_directory / "cpuacct").generic_string()
             << " cgroup rw,nosuid,nodev,noexec,relatime,cpuacct 0 0\n"
             << "cgroup " << (cgroup_directory / "memory").generic_string()
             << " cgroup rw,nosuid,nodev,noexec,relatime,memory 0 0\n";

        file.close();
    }

    static void writePids(const boost::filesystem::path& p,
                          const std::vector<pid_t>& pids) {
        std::ofstream file((p / "cgroup.procs").generic_string());
        for (const auto& pid : pids) {
            file << pid << std::endl;
        }
        file.close();
    }

    static void TearDownTestCase() {
        try {
            boost::filesystem::remove_all(test_directory);
        } catch (const std::exception& e) {
            std::cerr << "Failed to remove: " << test_directory.generic_string()
                      << ": " << e.what() << std::endl;
        }
    }

protected:
    void SetUp() override {
        instance = cb::cgroup::priv::make_control_group(
                test_directory.generic_string());
    }

public:
    static boost::filesystem::path test_directory;
    static boost::filesystem::path cgroup_directory;

    std::unique_ptr<cb::cgroup::ControlGroup> instance;
};

boost::filesystem::path MockControlGroup::test_directory;
boost::filesystem::path MockControlGroup::cgroup_directory;

class V1 : public MockControlGroup {
protected:
    void SetUp() override {
        writePids(cgroup_directory / "unified", {1, 2});
        writePids(cgroup_directory / "cpuacct", {getpid()});
        writePids(cgroup_directory / "cpu", {getpid()});
        writePids(cgroup_directory / "memory", {getpid()});
        MockControlGroup::SetUp();
    }
};

class V2 : public MockControlGroup {
protected:
    void SetUp() override {
        directory = cgroup_directory / "unified";
        writePids(directory, {getpid()});
        writePids(cgroup_directory / "cpu", {1, 2});
        writePids(cgroup_directory / "cpuacct", {1, 2});
        writePids(cgroup_directory / "memory", {1, 2});
        MockControlGroup::SetUp();
    }

    boost::filesystem::path directory;
};

/// We map to CGroup V1 if we fail to find the pid in any cgroup2
/// Verify that we behave correctly if the process isn't part of any
/// cgroup.procs file
class NoCgroupFound : public MockControlGroup {
protected:
    void SetUp() override {
        writePids(cgroup_directory / "unified", {1, 2});
        writePids(cgroup_directory / "cpu", {1, 2});
        writePids(cgroup_directory / "cpuacct", {1, 2});
        writePids(cgroup_directory / "memory", {1, 2});
        MockControlGroup::SetUp();
    }
};

class V1V2Mix : public MockControlGroup {
protected:
    void SetUp() override {
        directory = cgroup_directory / "unified";
        writePids(directory, {getpid()});
        writePids(cgroup_directory / "cpu", {1, 2, getpid()});
        writePids(cgroup_directory / "cpuacct", {1, 2, getpid()});
        writePids(cgroup_directory / "memory", {1, 2});
        MockControlGroup::SetUp();
    }

    boost::filesystem::path directory;
};

TEST_F(V1, TestCpuQuota) {
    EXPECT_LE(100, instance->get_available_cpu());

    // Now lets write a CPU quota file for 2 1/2 CPU. We use ceil so we should
    // get 3
    std::ofstream file(
            (cgroup_directory / "cpu" / "cpu.cfs_period_us").generic_string());
    file << "100000" << std::endl;
    file.close();
    file.open((cgroup_directory / "cpu" / "cpu.cfs_quota_us").generic_string());
    file << "250000" << std::endl;
    file.close();

    EXPECT_EQ(250, instance->get_available_cpu());
}

TEST_F(V1, TestMaxMemory) {
    EXPECT_EQ(0, instance->get_max_memory());

    std::ofstream file((cgroup_directory / "memory" / "memory.limit_in_bytes")
                               .generic_string());
    file << "17179869184" << std::endl;
    file.close();

    EXPECT_EQ(17179869184, instance->get_max_memory());
}

TEST_F(V1, TestCurrentMemory) {
    EXPECT_EQ(0, instance->get_current_memory());

    std::ofstream file((cgroup_directory / "memory" / "memory.usage_in_bytes")
                               .generic_string());
    file << "19111936" << std::endl;
    file.close();

    EXPECT_EQ(19111936, instance->get_current_memory());
}

TEST_F(V1, TestCpuStat) {
    std::ofstream file(
            (cgroup_directory / "cpuacct" / "cpuacct.stat").generic_string());
    file << "user 182" << std::endl << "system 54" << std::endl;
    file.close();

    file.open(
            (cgroup_directory / "cpuacct" / "cpuacct.usage").generic_string());
    file << "2815637074" << std::endl;
    file.close();

    file.open((cgroup_directory / "cpu" / "cpu.stat").generic_string());
    file << "nr_periods 331" << std::endl
         << "nr_throttled 5" << std::endl
         << "throttled_time 10000" << std::endl
         << "nr_bursts 10" << std::endl
         << "burst_time 100000" << std::endl;
    file.close();

    auto cpu = instance->get_cpu_stats();
    EXPECT_EQ(540000, cpu.system.count());
    EXPECT_EQ(1820000, cpu.user.count());
    EXPECT_EQ(2815637, cpu.usage.count());
    EXPECT_EQ(100, cpu.burst.count());
    EXPECT_EQ(10, cpu.nr_bursts);
    EXPECT_EQ(10, cpu.throttled.count());
    EXPECT_EQ(5, cpu.nr_throttled);
    EXPECT_EQ(331, cpu.nr_periods);
}

TEST_F(V2, TestCpuQuota) {
    auto current = instance->get_available_cpu();
    EXPECT_LE(100, current);

    std::ofstream file((directory / "cpu.max").generic_string());
    file << "max 100000" << std::endl;
    file.close();

    EXPECT_EQ(current, instance->get_available_cpu());

    file.open((directory / "cpu.max").generic_string());
    file << "150000 100000" << std::endl;
    file.close();

    EXPECT_EQ(150, instance->get_available_cpu());
}

TEST_F(V2, TestMaxMemory) {
    EXPECT_EQ(0, instance->get_max_memory());

    std::ofstream file((directory / "memory.max").generic_string());
    file << "max" << std::endl;
    file.close();
    EXPECT_EQ(0, instance->get_max_memory());

    file.open((directory / "memory.max").generic_string());
    file << "4294967296" << std::endl;
    file.close();

    EXPECT_EQ(4294967296, instance->get_max_memory());
}

TEST_F(V2, TestCurrentMemory) {
    EXPECT_EQ(0, instance->get_current_memory());

    std::ofstream file((directory / "memory.current").generic_string());
    file << "19111936" << std::endl;
    file.close();

    EXPECT_EQ(19111936, instance->get_current_memory());
}

TEST_F(V2, TestCpuStat) {
    std::ofstream file((directory / "cpu.stat").generic_string());
    file << "usage_usec 224892671" << std::endl
         << "user_usec 176621020" << std::endl
         << "system_usec 48271650" << std::endl
         << "nr_periods 38536" << std::endl
         << "nr_throttled 5" << std::endl
         << "throttled_usec 10" << std::endl
         << "nr_bursts 10" << std::endl
         << "burst_usec 113" << std::endl;

    auto cpu = instance->get_cpu_stats();
    EXPECT_EQ(48271650, cpu.system.count());
    EXPECT_EQ(176621020, cpu.user.count());
    EXPECT_EQ(224892671, cpu.usage.count());
    EXPECT_EQ(10, cpu.throttled.count());
    EXPECT_EQ(5, cpu.nr_throttled);
    EXPECT_EQ(38536, cpu.nr_periods);
    EXPECT_EQ(10, cpu.nr_bursts);
    EXPECT_EQ(113, cpu.burst.count());
}

TEST_F(NoCgroupFound, TestCpuQuota) {
    EXPECT_LE(100, instance->get_available_cpu());
}

TEST_F(NoCgroupFound, TestMaxMemory) {
    EXPECT_EQ(0, instance->get_max_memory());
}

TEST_F(NoCgroupFound, TestCurrentMemory) {
    EXPECT_EQ(0, instance->get_current_memory());
}

TEST_F(NoCgroupFound, TestCpuStat) {
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

TEST_F(V1V2Mix, V1ShouldBeSelected) {
    EXPECT_EQ(ControlGroup::Version::V1, instance->get_version());
}

/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/Synchronized.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>
#include <platform/process_monitor.h>
#include <filesystem>
#include <thread>
#include <vector>

#ifndef WIN32
#include <csignal>
#endif

static void testExitCode(int exitcode) {
    std::filesystem::path exe{std::filesystem::current_path() /
                              "process_monitor_child"};
    if (!std::filesystem::exists(exe)) {
        exe = exe.generic_string() + ".exe";
        if (!std::filesystem::exists(exe)) {
            FAIL() << "Failed to locate " << exe.generic_string();
        }
    }

    const auto lockfile =
            std::filesystem::path(cb::io::mktemp("./process_monitor."));
    const auto exitcodestring = std::to_string(exitcode);
    std::vector<std::string> argv = {{exe.generic_string(),
                                      "--lockfile",
                                      lockfile.generic_string(),
                                      "--exitcode",
                                      exitcodestring}};

    std::atomic_bool notified{false};
    auto child = ProcessMonitor::create(
            argv, [&notified](const auto&) { notified.store(true); });
    EXPECT_TRUE(child);
    EXPECT_TRUE(child->isRunning());
    EXPECT_FALSE(notified.load());
    remove(lockfile);
    const auto timeout =
            std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while (child->isRunning() && std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::microseconds{10});
    }
    EXPECT_LT(std::chrono::steady_clock::now(), timeout)
            << "Timeout waiting for the child to terminate";
    EXPECT_TRUE(notified.load());
    auto& ec = child->getExitCode();

    if (exitcode == EXIT_SUCCESS) {
        EXPECT_TRUE(ec.isSuccess()) << ec.to_string() << " " << ec.to_json();
        EXPECT_EQ("Success", ec.to_string());
    } else {
        EXPECT_FALSE(ec.isSuccess()) << ec.to_string() << " " << ec.to_json();
        EXPECT_EQ("Failure", ec.to_string());
    }
}

TEST(ProcessMonitorChild, Success) {
    testExitCode(EXIT_SUCCESS);
}

TEST(ProcessMonitorChild, Failure) {
    testExitCode(EXIT_FAILURE);
}

#ifndef WIN32
TEST(ProcessMonitorChild, Abort) {
    std::filesystem::path exe{std::filesystem::current_path() /
                              "process_monitor_child"};
    if (!std::filesystem::exists(exe)) {
        FAIL() << "Failed to locate " << exe.generic_string();
    }

    const auto lockfile =
            std::filesystem::path(cb::io::mktemp("./process_monitor."));
    std::vector<std::string> argv = {
            {exe.generic_string(), "--lockfile", lockfile.generic_string()}};

    auto child = ProcessMonitor::create(argv, [](const auto&) {});
    EXPECT_TRUE(child);
    EXPECT_TRUE(child->isRunning());

    auto descr = child->describe();
    ASSERT_EQ(0, kill(descr["pid"].get<int>(), SIGALRM));

    const auto timeout =
            std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while (child->isRunning() && std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::microseconds{10});
    }
    EXPECT_LT(std::chrono::steady_clock::now(), timeout)
            << "Timeout waiting for the child to terminate";
    // Clean up the lockfile
    remove(lockfile);

    auto& ec = child->getExitCode();
    EXPECT_FALSE(ec.isSuccess()) << ec.to_string() << " " << ec.to_json();
    auto json = ec.to_json();
    EXPECT_EQ(0, json["WCOREDUMP"]);
    EXPECT_EQ(0, json["WEXITSTATUS"]);
    EXPECT_EQ(false, json["WIFEXITED"]);
    EXPECT_EQ(true, json["WIFSIGNALED"]);
    EXPECT_EQ(SIGALRM, json["WTERMSIG"]);
}
#endif

/**
 * Test the monitor functionality for "parent" processes (it works
 * for all processes we have search rights for and not just parent
 * processes, so we may test it by creating a child and then use a
 * second monitor to monitor the child.
 */
TEST(ProcessMonitor, OtherProcess) {
    std::filesystem::path exe{std::filesystem::current_path() /
                              "process_monitor_child"};
    if (!std::filesystem::exists(exe)) {
        exe = exe.generic_string() + ".exe";
        if (!std::filesystem::exists(exe)) {
            FAIL() << "Failed to locate " << exe.generic_string();
        }
    }

    const auto lockfile =
            std::filesystem::path(cb::io::mktemp("./process_monitor."));
    std::vector<std::string> argv = {
            {exe.generic_string(), "--lockfile", lockfile.generic_string()}};

    std::atomic_bool child_notified{false};
    auto child = ProcessMonitor::create(argv, [&child_notified](const auto&) {
        child_notified.store(true);
    });

    // Create the monitor to watch the other process
    std::atomic_bool other_notified{false};
    auto other = ProcessMonitor::create(
            child->describe()["pid"].get<int>(),
            [&other_notified](const auto&) { other_notified.store(true); });

    EXPECT_TRUE(child->isRunning());
    EXPECT_TRUE(other->isRunning());

    // Tell the child to exit and verify that its cone
    remove(lockfile);

    const auto timeout =
            std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while ((child->isRunning() || other->isRunning()) &&
           std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::microseconds{10});
    }

    EXPECT_FALSE(child->isRunning());
    EXPECT_FALSE(other->isRunning());

    // And both should have been notified
    EXPECT_TRUE(child_notified);
    EXPECT_TRUE(other_notified);

    // The child monitor should return EXIT_SUCCESS
    EXPECT_TRUE(child->getExitCode().isSuccess());
    // The other monitor can't get the exit code and should return failure
    EXPECT_FALSE(other->getExitCode().isSuccess());
}

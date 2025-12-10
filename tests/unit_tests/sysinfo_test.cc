/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>
#include <folly/portability/Stdlib.h>
#include <platform/sysinfo.h>

TEST(GetAvailableCpu, NoVariable) {
    EXPECT_NE(0u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_ExactNumber) {
    setenv("COUCHBASE_CPU_COUNT", "10000", 1);
    EXPECT_EQ(10000u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_LeadingSpace) {
    setenv("COUCHBASE_CPU_COUNT", " 9999", 1);
    EXPECT_EQ(9999u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_TrailingSpace) {
    setenv("COUCHBASE_CPU_COUNT", "9998 ", 1);
    EXPECT_EQ(9998u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_LeadingTab) {
    setenv("COUCHBASE_CPU_COUNT", "\t9997", 1);
    EXPECT_EQ(9997u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_TrailigTab) {
    setenv("COUCHBASE_CPU_COUNT", "9996\t", 1);
    EXPECT_EQ(9996u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, InvalidValue) {
    setenv("COUCHBASE_CPU_COUNT", "1a", 1);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    setenv("COUCHBASE_CPU_COUNT", "1 a", 1);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    setenv("COUCHBASE_CPU_COUNT", "a1", 1);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    setenv("COUCHBASE_CPU_COUNT", "a 1", 1);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);
}

TEST(GetCpuCount, get_cpu_count) {
    auto count = cb::get_cpu_count();
    EXPECT_NE(0u, count);
    std::cout << "get_cpu_count:" << count << std::endl;
}

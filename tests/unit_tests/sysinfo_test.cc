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

#include <platform/checked_snprintf.h>
#include <platform/sysinfo.h>

char cpu_count_env_var[256];

TEST(GetAvailableCpu, NoVariable) {
    EXPECT_NE(0u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_ExactNumber) {
    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=10000");
    putenv(cpu_count_env_var);
    EXPECT_EQ(10000u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_LeadingSpace) {
    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT= 9999");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9999u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_TrailingSpace) {
    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=9998 ");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9998u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_LeadingTab) {
    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=\t9997");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9997u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_TrailigTab) {
    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=9996\t");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9996u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, InvalidValue) {
    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=1a");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=1 a");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=a1");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    checked_snprintf(cpu_count_env_var,
                     sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=a 1");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);
}

TEST(GetCpuCount, get_cpu_count) {
    auto count = cb::get_cpu_count();
    EXPECT_NE(0u, count);
    std::cout << "get_cpu_count:" << count << std::endl;
}

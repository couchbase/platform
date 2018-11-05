/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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
#include <folly/portability/GTest.h>

#include <platform/sysinfo.h>
#include <platform/checked_snprintf.h>

char cpu_count_env_var[256];

TEST(GetAvailableCpu, NoVariable) {
    EXPECT_NE(0u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_ExactNumber) {
    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=10000");
    putenv(cpu_count_env_var);
    EXPECT_EQ(10000u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_LeadingSpace) {
    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT= 9999");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9999u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_TrailingSpace) {
    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=9998 ");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9998u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_LeadingTab) {
    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=\t9997");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9997u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, CorrectVariable_TrailigTab) {
    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=9996\t");
    putenv(cpu_count_env_var);
    EXPECT_EQ(9996u, cb::get_available_cpu_count());
}

TEST(GetAvailableCpu, InvalidValue) {
    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=1a");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=1 a");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=a1");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);

    checked_snprintf(cpu_count_env_var, sizeof(cpu_count_env_var),
                     "COUCHBASE_CPU_COUNT=a 1");
    putenv(cpu_count_env_var);
    EXPECT_THROW(cb::get_available_cpu_count(), std::logic_error);
}

TEST(GetCpuCount, get_cpu_count) {
    auto count = cb::get_cpu_count();
    EXPECT_NE(0u, count);
    std::cout << "get_cpu_count:" << count << std::endl;
}

// This test may never fail, but we at least expect the following to never
// happen
TEST(GetCpuCount, get_cpu_index) {
    EXPECT_LE(cb::get_cpu_index(), cb::get_cpu_count());
}

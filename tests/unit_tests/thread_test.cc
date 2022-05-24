/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>
#include <folly/system/ThreadName.h>
#include <platform/thread.h>
#include <iostream>

TEST(ThreadnameTest, ThreadName) {
    EXPECT_TRUE(folly::canSetCurrentThreadName());
#ifndef WIN32
    // For some reason it seems like folly::getCurrentThreadName() isn't
    // working properly on Windows, and the thread names on windows is only
    // available for viewing in the debugger (not by the task manager AFAIK)
    // so lets just use folly for now (thread names appeared in Windows 10
    // b1607 so it might not be available on the target system anyway)
    auto getThreadName = []() -> std::string {
        auto name = folly::getCurrentThreadName();
        if (name.has_value()) {
            return *name;
        }
        return "getCurrentThreadName did not return a thread name";
    };

    EXPECT_TRUE(cb_set_thread_name("test"));
    EXPECT_EQ("test", getThreadName());

    try {
        std::string buffer;
        buffer.resize(80);
        std::fill(buffer.begin(), buffer.end(), 'a');
        cb_set_thread_name(buffer);
        FAIL() << "Should throw an exception";
    } catch (const std::logic_error&) {
    }
    // Check that a failing set thread name didn't mess up the value
    EXPECT_EQ("test", getThreadName());
#endif
}

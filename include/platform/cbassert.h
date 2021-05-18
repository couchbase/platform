/*
 *     Copyright 2013-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#ifdef __cplusplus
/**
 * If running on Windows with a Debug build, direct error and assertion
 * messages from the CRT to stderr, in addition to the default GUI dialog
 * box.
 * Also includes a backtrace of the error.
 * Ensures that errors from Debug-mode tests etc are visible even if running
 * in a non-graphical mode (e.g. Jenkins CV job).
 * No-op on non-Windows, non-Debug build.
 */
void setupWindowsDebugCRTAssertHandling();

extern "C" {
[[noreturn]]
#endif
void cb_assert_die(const char *expression, const char *file, int line);
#ifdef __cplusplus
}
#endif

#define cb_assert(e) \
    ((void)((e) ? (void)0 : cb_assert_die(#e, __FILE__, __LINE__)))

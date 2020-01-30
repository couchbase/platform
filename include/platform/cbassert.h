/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc.
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
#pragma once

#include <platform/dynamic.h>

#ifdef __cplusplus
extern "C" {
#endif
    void cb_assert_die(const char *expression, const char *file, int line)
        CB_ATTR_NORETURN;
#ifdef __cplusplus
}
#endif

#define cb_assert(e)  \
    ((void)((e) ? (void)0 : cb_assert_die(#e, __FILE__, __LINE__)))

/**
 * If running on Windows with a Debug build, direct error and assertion
 * messages from the CRT to stderr, in addition to the default GUI dialog
 * box.
 * Also includes a backtrace of the error.
 * Ensures that errors from Debug-mode tests etc are visible even if running
 * in a non-graphical mode (e.g. Jenkins CV job).
 * No-op on non-Windows, non-Debug build.
 */
#ifdef __cplusplus
extern "C" {
#endif
    void setupWindowsDebugCRTAssertHandling(void);
#ifdef __cplusplus
}
#endif

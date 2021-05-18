/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/backtrace.h>
#include <platform/cbassert.h>

#include <folly/portability/Windows.h>
#include <cstdio>
#include <cstdlib>

#if defined(WIN32)
#include <crtdbg.h>
#endif

void cb_assert_die(const char* expression, const char* file, int line) {
    fprintf(stderr, "assertion failed [%s] at %s:%u\n", expression, file, line);
    fprintf(stderr, "Called from:\n");
    print_backtrace_to_file(stderr);
    fflush(stderr);
    std::abort();
}

#if defined(WIN32)
int backtraceReportHook(int reportType, char* message, int* returnValue) {
    fprintf(stderr, message);
    fprintf(stderr, "Called from:\n");
    print_backtrace_to_file(stderr);
    return FALSE;
}

void setupWindowsDebugCRTAssertHandling() {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportHook(backtraceReportHook);
}
#else
void setupWindowsDebugCRTAssertHandling() {
}
#endif

/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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
#include <platform/backtrace.h>
#include <platform/cbassert.h>

#if defined(WIN32)
#include <crtdbg.h>
#include <folly/portability/Windows.h>
#endif

#include <cstdio>
#include <cstdlib>

#if defined(WIN32) || (defined(HAVE_BACKTRACE) && defined(HAVE_DLADDR))
#define HAVE_BACKTRACE_SUPPORT 1
#endif

#ifdef HAVE_BACKTRACE_SUPPORT
static void write_callback(void*, const char* frame) {
    fprintf(stderr, "\t%s\n", frame);
}
#endif

void cb_assert_die(const char* expression, const char* file, int line) {
    (void)fprintf(
            stderr, "assertion failed [%s] at %s:%u\n", expression, file, line);

#ifdef HAVE_BACKTRACE_SUPPORT
    fprintf(stderr, "Called from:\n");
    print_backtrace(write_callback, nullptr);
#endif

    fflush(stderr);
    abort();
}

#if defined(WIN32)

int backtraceReportHook(int reportType, char* message, int* returnValue) {
    fprintf(stderr, message);
    fprintf(stderr, "Called from:\n");
    print_backtrace(write_callback, nullptr);
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

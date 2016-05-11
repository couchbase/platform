/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

/* Test for print_backtrace - Setup a call stack of (at least) 3 frames,
 * then call print_backtrace; verifying that we get at least 3 frames.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <platform/backtrace.h>

#include <gtest/gtest.h>

#ifdef WIN32
    #define NOINLINE __declspec(noinline)
#else
    #define NOINLINE __attribute__((noinline))
#endif

NOINLINE static int leaf();
NOINLINE static int middle();
NOINLINE static int outer();

// Variable used in each function in the chain; to defeat tail-call
// optimization.
static int dummy;

// Count of how many frames we have seen.
static int frames;
static void* expected_ctx = (void*)0xcbdb;

static void write_callback(void* ctx, const char* frame) {
    cb_assert(ctx == expected_ctx);
    cb_assert(frame != NULL);
    cb_assert(strlen(frame) > 0);
    printf("%s\n", frame);
    frames++;
}

static int leaf() {
    print_backtrace(write_callback, expected_ctx);
    return dummy++;
}

static int middle() {
    leaf();
    return dummy++;
}

static int outer() {
    middle();
    return dummy++;
}

// Test the print_backtrace() function
TEST(BacktraceTest, PrintBacktrace) {
    outer();
    EXPECT_GE(frames, 3);
}

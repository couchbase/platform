/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <platform/exceptions.h>
#include <cstdio>
#include <functional>
#include <string>

using write_cb_t = void (*)(void*, const char*);

/**
 * Prints a backtrace from the current thread. For each frame, the
 * `write_cb` function is called with `context` and a string describing
 * the frame.
 */
void print_backtrace(write_cb_t write_cb, void* context);

void print_backtrace_frames(const boost::stacktrace::stacktrace& frames,
                            std::function<void(const char* frame)> callback);

/**
 * Convenience function - prints a backtrace to the specified FILE.
 */
void print_backtrace_to_file(FILE* stream);

/**
 * print a backtrace to a buffer
 *
 * @param indent the indent used for each entry in the callstack
 * @param buffer the buffer to populate with the backtrace
 * @param size the size of the input buffer
 */
bool print_backtrace_to_buffer(const char *indent, char *buffer, size_t size);

namespace cb::backtrace {
/**
 * Prepare the process for being able to call the backtrace methods.
 *
 * On Windows we need to load the symbol tables and from the looks of it
 * that won't work if we try to do it after we crashed (in the signal
 * handler).
 */
void initialize();

/**
 * Get the current backtrace as a string.
 *
 * @return a string containing the backtrace
 */
[[nodiscard]] std::string current();
}

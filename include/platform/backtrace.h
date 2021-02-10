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
#pragma once

#include <platform/exceptions.h>
#include <cstdio>
#include <functional>

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
}

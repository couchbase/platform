/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
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

/* Global replacement of operators new and delete.
 *
 * By linking this file into a binary we globally replace new & delete.
 *
 * This allows us to track how much memory has been allocated from C++ code,
 * both in total and per ep-engine instance, by allowing interested parties to
 * register hook functions for new and delete.
 *
 * Usage:
 *
 * Link this file into /all/ binaries (executables, shared libraries and DLLs)
 * for which you want to have memory tracking for C++ allocations.
 *
 * (Note: On most *ix-like platforms (Linux and OS X tested), it is sufficient
 *        to link this file into the main executable (e.g. `memcached`) and
 *        all shared libraries and dlopen'ed plugins will use these functions
 *        automagically. However this isn't the case on Windows/MSVC, where you
 *        need to link into all binaries for the overrider to take effect.
 *
 * Implementation:
 *
 * According to the C++11 spec
 * (http://en.cppreference.com/w/cpp/memory/new/operator_new#Global_replacements),
 * overriding base `operator new` and `operator delete` /should/ be sufficient
 * to all C++ allocations, however this isn't true on Windows/MSVC, where we
 * also need to override the array forms.
 */

#include "config.h"

#include <platform/cb_malloc.h>

#include <cstdlib>
#include <new>

#if defined(WIN32)
#  define NOEXCEPT
#else
#  define NOEXCEPT noexcept
#endif

// Silence GCCs warning about redundant redeclaration (again, this will go
// away soon).
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
void* operator new(std::size_t count);
void operator delete(void* ptr) NOEXCEPT;
#pragma GCC diagnostic pop

void* operator new(std::size_t count) {
    void* result = cb_malloc(count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

void operator delete(void* ptr) NOEXCEPT {
    cb_free(ptr);
}

#if defined(WIN32)
// Also need to override the array forms for Windows/MSVC
void* operator new[](std::size_t count) {
    void* result = cb_malloc(count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

void operator delete[](void *ptr) {
    cb_free(ptr);
}
#endif

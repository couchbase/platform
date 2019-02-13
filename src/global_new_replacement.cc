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
 * From C++11 onwards overriding base `operator new` and `operator delete`
 * /should/ be sufficient to all C++ allocations, however this isn't true on
 * Windows/MSVC, where we also need to override the array forms.
 * http://en.cppreference.com/w/cpp/memory/new/operator_new#Global_replacements
 *
 * However, since our allocator of choice is jemalloc, who decided to overload
 * the new[] and delete[] operators since version 5.1.0 as part of sized
 * deallocation, we need to overload the array form operators on all platforms.
 * https://github.com/jemalloc/jemalloc/commit/2319152d9f5d9b33eebc36a50ccf4239f31c1ad9
 */

#include "config.h"

#include <platform/cb_malloc.h>

#include <cstdlib>
#include <new>

#if defined(WIN32)
#  define NOEXCEPT
#else
#  define NOEXCEPT noexcept

// Silence GCCs warning about redundant redeclaration (again, this will go
// away soon).
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
void* operator new(std::size_t count);
void operator delete(void* ptr) NOEXCEPT;
#pragma GCC diagnostic pop

#endif

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

void operator delete(void* ptr, std::size_t size) NOEXCEPT {
    cb_free(ptr);
}

void* operator new[](std::size_t count) {
    void* result = cb_malloc(count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

void operator delete[](void *ptr) NOEXCEPT {
    cb_free(ptr);
}

void operator delete[](void *ptr, std::size_t size) NOEXCEPT {
    cb_free(ptr);
}

/* As we have a global new replacement, libraries could end up calling the
 * system malloc_usable_size (if present) with a pointer to memory
 * allocated by different allocator. This interposes malloc_usable_size
 * to ensure the malloc_usable_size of the desired allocator is called.
 *
 * In the case where our desired allocator is the system allocator, there exists
 * an issue in the cb_malloc_usable_size implementation where, because the
 * malloc_usable_size call is not prefixed, we get stuck in an infinite
 * recursive loop causing a stack overflow as our attempt to forward on the
 * malloc_usable_size call brings us back to this overload. This can be fixed
 * by only defining this overload if we are not using the system allocator.
 * This was spotted under TSan where we use the system allocator. For some
 * unknown reason, when we define the sized delete operator overload the runtime
 * linking order changes. We now link the new, delete, and malloc_usable_size
 * symbols in this file to a test suite before we link the operators in RocksDB.
 * This causes RocksDB to link to the symbols in this file, whereas previously
 * it would link directly to TSan's overloaded operators.
 */
#if defined(HAVE_MALLOC_USABLE_SIZE) and !defined(HAVE_SYSTEM_MALLOC)
extern "C" PLATFORM_PUBLIC_API size_t malloc_usable_size(void* ptr) {
    return cb_malloc_usable_size(ptr);
}
#endif

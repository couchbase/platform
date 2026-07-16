/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
 * From C++17 onwards there are multiple variants of overridable
 * `operator new` and `operator delete` - see
 * http://en.cppreference.com/w/cpp/memory/new/operator_new#Global_replacements
 * We must override all variants, mapping to the appropriate cb_malloc etc
 * function so memory usage is correctly accounted.
 */

#include <platform/cb_malloc.h>
#include <cstdlib>
#include <new>

#if !defined(WIN32) && (__cplusplus > 202302L)
#warning Only operator new/delete up to C++23 overridden. If later standards add additional overrides they should be added here.
#endif

// New operators numbered from:
// https://en.cppreference.com/w/cpp/memory/new/operator_new

// (new 1)
[[nodiscard]] void* operator new(std::size_t count) {
    void* result = cb_malloc(count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

// (new 2)
[[nodiscard]] void* operator new[](std::size_t count) {
    void* result = cb_malloc(count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

// (new 3)
[[nodiscard]] void* operator new(std::size_t count, std::align_val_t al) {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

// (new 4)
[[nodiscard]] void* operator new[](std::size_t count, std::align_val_t al) {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

// (new 5)
[[nodiscard]] void* operator new(std::size_t count,
                                 const std::nothrow_t& tag) noexcept {
    void* result = cb_malloc(count);
    return result;
}

// (new 6)
[[nodiscard]] void* operator new[](std::size_t count,
                                   const std::nothrow_t& tag) noexcept {
    void* result = cb_malloc(count);
    return result;
}

// (new 7)
[[nodiscard]] void* operator new(std::size_t count,
                                 std::align_val_t al,
                                 const std::nothrow_t& tag) noexcept {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    return result;
}

// (new 8)
[[nodiscard]] void* operator new[](std::size_t count,
                                   std::align_val_t al,
                                   const std::nothrow_t& tag) noexcept {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    return result;
}

// Delete operators numbered from:
// https://en.cppreference.com/w/cpp/memory/new/operator_delete

// (del 1)
void operator delete(void* ptr) noexcept {
    cb_free(ptr);
}

// (del 2)
void operator delete[](void* ptr) noexcept {
    cb_free(ptr);
}

// (del 3)
void operator delete(void* ptr, std::align_val_t) noexcept {
    cb_aligned_free(ptr);
}

// (del 4)
void operator delete[](void* ptr, std::align_val_t) noexcept {
    cb_aligned_free(ptr);
}

// (del 6)
void operator delete[](void* ptr, std::size_t size) noexcept {
    cb_sized_free(ptr, size);
}

// (del 5)
void operator delete(void* ptr, std::size_t size) noexcept {
    cb_sized_free(ptr, size);
}

// (del 7)
void operator delete(void* ptr, std::size_t size, std::align_val_t) noexcept {
    cb_aligned_free(ptr);
}

// (del 8)
void operator delete[](void* ptr, std::size_t size, std::align_val_t) noexcept {
    cb_aligned_free(ptr);
}

// (del 9)
void operator delete(void* ptr, const std::nothrow_t& tag) noexcept {
    cb_free(ptr);
}

// (del 10)
void operator delete[](void* ptr, const std::nothrow_t& tag) noexcept {
    cb_free(ptr);
}

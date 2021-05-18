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
#if __cplusplus > 201703L
#warning Only operator new/delete up to C++17 overridden. If later standards add additional overrides they should be added here.
#endif

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

void* operator new(std::size_t count, std::align_val_t al) {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

void* operator new(std::size_t count, const std::nothrow_t& tag) NOEXCEPT {
    void* result = cb_malloc(count);
    return result;
}

void* operator new(std::size_t count,
                   std::align_val_t al,
                   const std::nothrow_t& tag) NOEXCEPT {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    return result;
}

void operator delete(void* ptr) NOEXCEPT {
    cb_free(ptr);
}

void operator delete(void* ptr, std::align_val_t) NOEXCEPT {
    cb_aligned_free(ptr);
}

void operator delete(void* ptr, const std::nothrow_t& tag) NOEXCEPT {
    cb_free(ptr);
}

void operator delete(void* ptr, std::size_t size) NOEXCEPT {
    cb_sized_free(ptr, size);
}

void operator delete(void* ptr, std::size_t size, std::align_val_t) NOEXCEPT {
    cb_aligned_free(ptr);
}

void* operator new[](std::size_t count) {
    void* result = cb_malloc(count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

void* operator new[](std::size_t count, std::align_val_t al) {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    return result;
}

void* operator new[](std::size_t count, const std::nothrow_t& tag) NOEXCEPT {
    void* result = cb_malloc(count);
    return result;
}

void* operator new[](std::size_t count,
                     std::align_val_t al,
                     const std::nothrow_t& tag) NOEXCEPT {
    void* result = cb_aligned_alloc(static_cast<size_t>(al), count);
    return result;
}

void operator delete[](void *ptr) NOEXCEPT {
    cb_free(ptr);
}

void operator delete[](void *ptr, std::align_val_t) NOEXCEPT {
    cb_aligned_free(ptr);
}

void operator delete[](void *ptr, std::size_t size) NOEXCEPT {
    cb_sized_free(ptr, size);
}

void operator delete[](void *ptr, std::size_t size, std::align_val_t) NOEXCEPT {
    cb_aligned_free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t& tag) NOEXCEPT {
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
#if !defined(HAVE_SYSTEM_MALLOC)
extern "C"
#if WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
size_t malloc_usable_size(void* ptr);

size_t malloc_usable_size(void* ptr) {
    return cb_malloc_usable_size(ptr);
}
#endif

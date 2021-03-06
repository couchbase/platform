/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cstdlib>
#include <cstring>

// Which underlying memory allocator should we use?
#if defined(HAVE_JEMALLOC)
#define MALLOC_PREFIX je_
/* Irrespective of how jemalloc was configured on this platform,
 * don't rename je_FOO to FOO.
 */
#define JEMALLOC_NO_RENAME
#include <jemalloc/jemalloc.h>

#else /* system allocator */
#define MALLOC_PREFIX
#include <folly/portability/Malloc.h>
#endif

extern "C" {

// When building with GCC/Clang we will define cb_malloc et al as a weak
// symbol and define functions that map to cstdlib memory allocator methods.
void* __attribute__((weak)) cb_malloc(size_t);
void* __attribute__((weak)) cb_calloc(size_t, size_t);
void* __attribute__((weak)) cb_realloc(void*, size_t);
void* __attribute__((weak)) cb_aligned_alloc(size_t, size_t);
void __attribute__((weak)) cb_free(void*);
void __attribute__((weak)) cb_aligned_free(void*);
void __attribute__((weak)) cb_sized_free(void*, size_t);
size_t __attribute__((weak)) cb_malloc_usable_size(void*);
char* __attribute__((weak)) cb_strdup(const char*);
int __attribute__((weak)) cb_malloc_is_using_arenas();
const char* __attribute__((weak)) cb_malloc_get_conf();

// Macros to form the name of the memory allocation function to use based on
// the compile-time selected malloc library's prefix ('je_', 'tc_', '')
#define CONCAT(A, B) A##B
#define CONCAT2(A, B) CONCAT(A, B)
#define MEM_ALLOC(name) CONCAT2(MALLOC_PREFIX, name)

void* cb_malloc(size_t size) {
    return MEM_ALLOC(malloc)(size);
}

void* cb_calloc(size_t count, size_t size) {
    return MEM_ALLOC(calloc)(count, size);
}

void* cb_realloc(void* p, size_t size) {
    return MEM_ALLOC(realloc)(p, size);
}

void* cb_aligned_alloc(size_t align, size_t size) {
#if defined(HAVE_JEMALLOC) || defined(HAVE_ALIGNED_ALLOC)
    // Can directly use aligned_alloc function from malloc library.
    return MEM_ALLOC(aligned_alloc)(align, size);
#elif defined(HAVE_POSIX_MEMALIGN)
    void* newAlloc = nullptr;
    if (posix_memalign(&newAlloc, align, size)) {
        newAlloc = nullptr;
    }
    return newAlloc;
#else
#error No underlying API for aligned memory available.
#endif
}

void cb_free(void* p) {
    MEM_ALLOC(free)(p);
}

void cb_aligned_free(void* p) {
#if defined(HAVE_JEMALLOC) || !defined(WIN32)
    MEM_ALLOC(free)(p);
    // Apart from Win32 without jemalloc, can use free() function from malloc
    // library.
#else
    _aligned_free(p);
#endif
}

void cb_sized_free(void* p, size_t size) {
#if defined(HAVE_JEMALLOC_SDALLOCX)
    if (p != nullptr) {
        MEM_ALLOC(sdallocx)(p, size, /* no flags */ 0);
    }
#else
    MEM_ALLOC(free)(p);
#endif
}

char* cb_strdup(const char* c) {
    size_t len = std::strlen(c);
    char* result = static_cast<char*>(cb_malloc(len + 1));
    if (result != nullptr) {
        std::strncpy(result, c, len + 1);
    }
    return result;
}

size_t cb_malloc_usable_size(void* ptr) {
    return MEM_ALLOC(malloc_usable_size)(ptr);
}

int cb_malloc_is_using_arenas() {
    return 0;
}

const char* cb_malloc_get_conf() {
    return "";
}

} // extern "C"

#undef MALLOC_PREFIX
#undef CONCAT
#undef CONCAT2
#undef MEM_ALLOC

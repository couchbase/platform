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

#include <platform/cb_malloc.h>

#include <cstring>

// Which underlying memory allocator should we use?
#if defined(HAVE_JEMALLOC)
#  if defined(__APPLE__)
/* On OS X, jemalloc has no new/delete hooks itself, and we instead rely
 * on our own custom zone (darwin_zone.c) to intercept malloc/realloc etc and
 * call the memory hook (before chaining to the real allocator).
 * Therefore even with je_malloc selected we still need to call 'normal' malloc
 * function - if we called je_malloc directly we wouldn't invoke our custom
 * hooks.
 */
#    define MALLOC_PREFIX
#  else
#    define MALLOC_PREFIX je_
#  endif // defined(__APPLE__)
/* Irrespective of how jemalloc was configured on this platform,
 * don't rename je_FOO to FOO.
 */
#  define JEMALLOC_NO_RENAME
#  include <jemalloc/jemalloc.h>

#elif defined(HAVE_TCMALLOC)
#  define MALLOC_PREFIX tc_
#  include <gperftools/tcmalloc.h>

#else /* system allocator */
#  define MALLOC_PREFIX
#endif


// User-registered new and delete hooks
cb_malloc_new_hook_t cb_new_hook = nullptr;
cb_malloc_delete_hook_t cb_delete_hook = nullptr;

/*
 * Memory allocation functions - equivalent to the libc functions with matching
 * names.
 */
#if defined(__sun) || defined(__FreeBSD__)
#define throwspec
#else
#define throwspec throw()
#endif

// Macros to form the name of the memory allocation function to use based on
// the compile-time selected malloc library's prefix ('je_', 'tc_', '')
#define CONCAT(A, B) A ## B
#define CONCAT2(A, B) CONCAT(A, B)
#define MEM_ALLOC(name) CONCAT2(MALLOC_PREFIX, name)

PLATFORM_PUBLIC_API void* cb_malloc(size_t size) throwspec {
    void* ptr = MEM_ALLOC(malloc)(size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

PLATFORM_PUBLIC_API void* cb_calloc(size_t nmemb, size_t size) throwspec {
    void* ptr = MEM_ALLOC(calloc)(nmemb, size);
    cb_invoke_new_hook(ptr, nmemb * size);
    return ptr;
}

PLATFORM_PUBLIC_API void* cb_realloc(void* ptr, size_t size) throwspec {
    cb_invoke_delete_hook(ptr);
    void* result = MEM_ALLOC(realloc)(ptr, size);
    cb_invoke_new_hook(result, size);
    return result;
}

PLATFORM_PUBLIC_API void cb_free(void* ptr) throwspec {
    cb_invoke_delete_hook(ptr);
    return MEM_ALLOC(free)(ptr);
}

PLATFORM_PUBLIC_API char* cb_strdup(const char* s1) {
    size_t len = std::strlen(s1);
    char* result = static_cast<char*>(cb_malloc(len + 1));
    if (result != nullptr) {
        std::strncpy(result, s1, len + 1);
    }
    return result;
}

/*
 * Allocation / deallocation hook functions.
 */
bool cb_add_new_hook(cb_malloc_new_hook_t f) {
    if (cb_new_hook == nullptr) {
        cb_new_hook = f;
        return true;
    } else {
        return false;
    }
}

bool cb_remove_new_hook(cb_malloc_new_hook_t f) {
    if (cb_new_hook == f) {
        cb_new_hook = nullptr;
        return true;
    } else {
        return false;
    }
}

bool cb_add_delete_hook(cb_malloc_delete_hook_t f) {
    if (cb_delete_hook == nullptr) {
        cb_delete_hook = f;
        return true;
    } else {
        return false;
    }
}

bool cb_remove_delete_hook(cb_malloc_delete_hook_t f) {
    if (cb_delete_hook == f) {
        cb_delete_hook = nullptr;
        return true;
    } else {
        return false;
    }
}

void cb_invoke_new_hook(const void* ptr, size_t size) {
    if (cb_new_hook != nullptr) {
        cb_new_hook(ptr, size);
    }
}

void cb_invoke_delete_hook(const void* ptr) {
    if (cb_delete_hook != nullptr) {
        cb_delete_hook(ptr);
    }
}

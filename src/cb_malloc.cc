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

#include <platform/cb_arena_malloc.h>
#include <platform/cb_malloc.h>

#include <cstring>

// Which underlying memory allocator should we use?
#if defined(HAVE_JEMALLOC)
/* Irrespective of how jemalloc was configured on this platform,
 * don't rename je_FOO to FOO.
 */
#  define JEMALLOC_NO_RENAME
#  include <jemalloc/jemalloc.h>

#else /* system allocator */
#if defined(HAVE_MALLOC_USABLE_SIZE)
#include <malloc.h>
#endif
#endif

#include <string>

// User-registered new and delete hooks, these are generally null except for
// test code.
cb_malloc_new_hook_t cb_new_hook = nullptr;
cb_malloc_delete_hook_t cb_delete_hook = nullptr;

PLATFORM_PUBLIC_API void* cb_malloc(size_t size) throw() {
    void* ptr = cb::ArenaMalloc::malloc(size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

PLATFORM_PUBLIC_API void* cb_calloc(size_t nmemb, size_t size) throw() {
    void* ptr = cb::ArenaMalloc::calloc(nmemb, size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

PLATFORM_PUBLIC_API void* cb_realloc(void* ptr, size_t size) throw() {
    cb_invoke_delete_hook(ptr);
    ptr = cb::ArenaMalloc::realloc(ptr, size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

PLATFORM_PUBLIC_API void cb_free(void* ptr) throw() {
    cb_invoke_delete_hook(ptr);
    cb::ArenaMalloc::free(ptr);
}

PLATFORM_PUBLIC_API void cb_sized_free(void* ptr, size_t size) throw() {
    cb_invoke_delete_hook(ptr);
    cb::ArenaMalloc::sized_free(ptr, size);
}

PLATFORM_PUBLIC_API char* cb_strdup(const char* s1) {
    size_t len = std::strlen(s1);
    char* result = static_cast<char*>(cb_malloc(len + 1));
    if (result != nullptr) {
        std::strncpy(result, s1, len + 1);
    }
    return result;
}

#if defined(HAVE_MALLOC_USABLE_SIZE)
PLATFORM_PUBLIC_API size_t cb_malloc_usable_size(void* ptr) throw() {
    return cb::ArenaMalloc::malloc_usable_size(ptr);
}
#endif

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

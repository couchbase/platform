/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/cb_arena_malloc.h>
#include <platform/cb_malloc.h>

#include <cstring>

// MB-38422: There is no je_malloc_conf for windows
#if defined(HAVE_JEMALLOC) && !defined(WIN32)
extern const char* je_malloc_conf;
#endif

// User-registered new and delete hooks, these are generally null except for
// test code.
cb_malloc_new_hook_t cb_new_hook = nullptr;
cb_malloc_delete_hook_t cb_delete_hook = nullptr;

static inline void cb_invoke_new_hook(const void* ptr, size_t size) {
    if (cb_new_hook != nullptr) {
        cb_new_hook(ptr, size);
    }
}

static inline void cb_invoke_delete_hook(const void* ptr) {
    if (cb_delete_hook != nullptr) {
        cb_delete_hook(ptr);
    }
}

void* cb_malloc(size_t size) noexcept {
    void* ptr = cb::ArenaMalloc::malloc(size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

void* cb_calloc(size_t nmemb, size_t size) noexcept {
    void* ptr = cb::ArenaMalloc::calloc(nmemb, size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

void* cb_realloc(void* ptr, size_t size) noexcept {
    cb_invoke_delete_hook(ptr);
    ptr = cb::ArenaMalloc::realloc(ptr, size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

void* cb_aligned_alloc(size_t alignment, size_t size) noexcept {
    void* ptr = cb::ArenaMalloc::aligned_alloc(alignment, size);
    cb_invoke_new_hook(ptr, size);
    return ptr;
}

void cb_free(void* ptr) noexcept {
    cb_invoke_delete_hook(ptr);
    cb::ArenaMalloc::free(ptr);
}

void cb_aligned_free(void* ptr) noexcept {
    cb_invoke_delete_hook(ptr);
    cb::ArenaMalloc::aligned_free(ptr);
}

void cb_sized_free(void* ptr, size_t size) noexcept {
    cb_invoke_delete_hook(ptr);
    cb::ArenaMalloc::sized_free(ptr, size);
}

char* cb_strdup(const char* s1) {
    size_t len = std::strlen(s1);
    char* result = static_cast<char*>(cb_malloc(len + 1));
    if (result != nullptr) {
        std::strncpy(result, s1, len + 1);
    }
    return result;
}

size_t cb_malloc_usable_size(void* ptr) noexcept {
    return cb::ArenaMalloc::malloc_usable_size(ptr);
}

int cb_malloc_is_using_arenas() {
    return 1;
}

const char* cb_malloc_get_conf() {
    // MB-38422: There is no je_malloc_conf for windows
#if defined(HAVE_JEMALLOC) && !defined(WIN32)
    return je_malloc_conf;
#else
    return "";
#endif
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

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

/** This file defines the Couchbase memory allocation API.
 *
 * It doesn't implement allocation itself - the actual memory allocation will
 * be performed by an exising 'proper' memory allocator. It exists for
 * two reasons:
 *
 * 1. To give us a single 'interposing' point to use an alternative
 *    allocator (e.g. jemalloc) instead of the system one.
 *    (On most *ix platforms it's relatively easy to interpose malloc and
 *    friends; you can simply define your own symbols in the application binary
 *    and those will be used; however this isn't possible on Windows so we
 *    need our own explicit API for the C memory allocation functions).
 *
 * 2. To allow us to insert hooks for memory tracking - e.g. so we can track
 *    how much memory each bucket (aka engine instance) are using.
 */

#pragma once

#include "config.h"

#include <platform/platform.h>

#include <stdlib.h>

#if !defined(__cplusplus) || defined(__sun) || defined(__FreeBSD__)
#define throwspec
#else
#define throwspec throw()
#endif

/*
 * Couchbase memory allocation API functions.
 *
 * Equivalent to the libc functions with the same suffix.
 */

#ifdef __cplusplus
extern "C" {
#endif

PLATFORM_PUBLIC_API void* cb_malloc(size_t size) throwspec;
PLATFORM_PUBLIC_API void* cb_calloc(size_t nmemb, size_t size) throwspec;
PLATFORM_PUBLIC_API void* cb_realloc(void* ptr, size_t size) throwspec;
PLATFORM_PUBLIC_API void cb_free(void* ptr) throwspec;

#if defined(HAVE_MALLOC_USABLE_SIZE)
PLATFORM_PUBLIC_API size_t cb_malloc_usable_size(void* ptr) throwspec;
#endif

#undef throwspec

/*
 * Replacements for other libc functions which allocate memory via 'malloc'.
 *
 * For our 'cb' versions we use cb_malloc instead.
 */

PLATFORM_PUBLIC_API char* cb_strdup(const char* s1);

#ifdef __cplusplus
} // extern "C"
#endif


#if defined(__cplusplus)
/*
 * Memory allocation / deallocation hook support.
 *
 * The following functions allow callbacks (hooks) to be registered, which
 * are then called on every memory allocation and deallocation.
 *
 * Only one hook for each of new & delete can be installed at any time.
 * Note these are *not* MT-safe - clients should only ever add/remove hooks
 * from a single thread (or use external locking).
 */

/**
 * Callback prototype for new hook. This is called /after/ the memory has
 * been allocated by the underlying allocator.
 *
 * @param ptr Address of memory allocated
 * @param sz Size in bytes of requested allocation.
 */
typedef void(*cb_malloc_new_hook_t)(const void *ptr, size_t sz);

/**
 * Callback prototype for delete hook. This is called /before/ the memory has
 * been freed by the underlying allocator.
 *
 * @param ptr Address of memory to be deallocated.
 */
typedef void(*cb_malloc_delete_hook_t)(const void *ptr);

PLATFORM_PUBLIC_API bool cb_add_new_hook(cb_malloc_new_hook_t f);
PLATFORM_PUBLIC_API bool cb_remove_new_hook(cb_malloc_new_hook_t f);
PLATFORM_PUBLIC_API bool cb_add_delete_hook(cb_malloc_delete_hook_t f);
PLATFORM_PUBLIC_API bool cb_remove_delete_hook(cb_malloc_delete_hook_t f);

/* Functions to call the new / delete hooks; if they are non-null. */
PLATFORM_PUBLIC_API void cb_invoke_new_hook(const void* ptr, size_t size);

PLATFORM_PUBLIC_API void cb_invoke_delete_hook(const void* ptr);

#endif // __cplusplus

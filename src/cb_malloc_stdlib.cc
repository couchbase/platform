/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2020 Couchbase, Inc
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

#include <platform/visibility.h>

#include <cstdlib>
#include <cstring>
#if defined(HAVE_MALLOC_USABLE_SIZE)
#include <malloc.h>
#endif

extern "C" {

// When building with GCC/Clang we will define cb_malloc et al as a weak
// symbol and define functions that map to cstdlib memory allocator methods.
PLATFORM_PUBLIC_API
void* __attribute__((weak)) cb_malloc(size_t);
PLATFORM_PUBLIC_API
void* __attribute__((weak)) cb_calloc(size_t, size_t);
PLATFORM_PUBLIC_API
void* __attribute__((weak)) cb_realloc(void*, size_t);
PLATFORM_PUBLIC_API
void __attribute__((weak)) cb_free(void*);
PLATFORM_PUBLIC_API
void __attribute__((weak)) cb_sized_free(void*, size_t);
#if defined(HAVE_MALLOC_USABLE_SIZE)
PLATFORM_PUBLIC_API
size_t __attribute__((weak)) cb_malloc_usable_size(void*);
#endif
PLATFORM_PUBLIC_API
char* __attribute__((weak)) cb_strdup(const char*);
PLATFORM_PUBLIC_API
int __attribute__((weak)) cb_malloc_is_using_arenas();

void* cb_malloc(size_t size) {
    return malloc(size);
}

void* cb_calloc(size_t count, size_t size) {
    return calloc(count, size);
}

void* cb_realloc(void* p, size_t size) {
    return realloc(p, size);
}

void cb_free(void* p) {
    free(p);
}

void cb_sized_free(void* p, size_t) {
    free(p);
}

char* cb_strdup(const char* c) {
    return strdup(c);
}

#if defined(HAVE_MALLOC_USABLE_SIZE)
size_t cb_malloc_usable_size(void* ptr) {
    return malloc_usable_size(ptr);
}
#endif

int cb_malloc_is_using_arenas() {
    return 0;
}

} // extern "C"

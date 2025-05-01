/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include "platform/cb_arena_malloc.h"
#include "platform/cb_malloc.h"
namespace cb {

/**
 * Defines a type template called Allocator which allocates via cb_malloc, but
 * always uses a guard to ensure that the memory is allocated/freed from the
 * correct arena.
 */
template <typename G>
struct AllocatorBuilder {
    using Guard = G;

    template <typename T>
    struct Allocator {
        using value_type = T;
        Allocator() = default;
        // Required for MSVC, see:
        // https://learn.microsoft.com/en-us/cpp/standard-library/allocators?view=msvc-170#code-try-1
        // Otherwise, some std::basic_string constructors become unavailable.
        template <typename U>
        Allocator(const Allocator<U>&) noexcept {
        }
        /**
         * Allocates memory for n elements of type T.
         */
        T* allocate(size_t n) const {
            Guard guard;
            return static_cast<T*>(cb_malloc(sizeof(T) * n));
        }
        /**
         * Releases memory for n elements of type T.
         */
        void deallocate(T* ptr, size_t n) const {
            Guard guard;
            cb_sized_free(ptr, sizeof(T) * n);
        }
        /**
         * This type has no state, so all instances compare equal.
         */
        friend constexpr bool operator==(Allocator, Allocator) {
            return true;
        }
        friend constexpr bool operator!=(Allocator, Allocator) {
            return false;
        }
    };
};

template <typename T>
using NoArenaAllocator =
        typename AllocatorBuilder<NoArenaGuard>::template Allocator<T>;

} // namespace cb

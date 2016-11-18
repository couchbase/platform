/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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
#pragma once

// Range (in bytes) we consider false sharing can occur. You may
// expect this to be a single cache line (64B on x86-64), but on
// Sandybridge (at least) it has been observed that pairs of
// cachelines can interfere with each other.
#define FALSE_SHARING_RANGE 128

namespace cb {

/**
 * Holds a type T, in addition to enough padding to round the size up to the
 * next multiple of the cacheline size.
 *
 * Based on folly's CachelinePadded
 * (https://github.com/facebook/folly/blob/master/folly/CachelinePadded.h).
 *
 * If T is standard-layout, then casting a T* you get from this class to a
 * CachelinePadded<T>* is safe.
 *
 */
template <typename T>
class
#if !defined(_MSC_VER) || _MSC_VER >= 1900 /* Non-MSVC, or
                                              Visual Studio 2015 upwards*/
// Align to the larger of sizeof(T) and FALSE_SHARING_RANGE
alignas(T) alignas(FALSE_SHARING_RANGE)
#endif
CachelinePadded {
public:
    template <typename... Args>
    explicit CachelinePadded(Args&&... args)
        : item(std::forward<Args>(args)...) {}

    CachelinePadded() {}

    T* get() {
      return &item;
    }

    const T* get() const {
      return &item;
    }

    T* operator->() {
      return get();
    }

    const T* operator->() const {
      return get();
    }

    T& operator*() {
      return *get();
    }

    const T& operator*() const {
      return *get();
    }

private:
#if defined(_MSC_VER) && _MSC_VER < 1900
    /* prior to Visual Studio 2015 cannot set alignment on a template class,
       so just align the item member.*/
  __declspec(align(FALSE_SHARING_RANGE))
#endif
  T item;
};

} // namespace cb

#undef FALSE_SHARING_RANGE

/// output stream operator - forwards to underlying type's operator to
/// allow generic code to still stream padded types.
template<typename T>
std::ostream& operator<<(std::ostream& os, const cb::CachelinePadded<T>& obj)
{
    os << *obj;
    return os;
}

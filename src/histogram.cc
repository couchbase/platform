/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
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

#include <platform/histogram.h>

/*
 * Histogram<> definitions of large methods which we prefer to not inline.
 */

template <typename T, template <class> class Limits>
void Histogram<T, Limits>::add(T amount, size_t count) {
    (*findBin(amount))->incr(count);
}

template <typename T, template <class> class Limits>
void Histogram<T, Limits>::reset() {
    std::for_each(bins.begin(), bins.end(),
                  [](value_type& val){val->set(0);});
}

template <typename T, template <class> class Limits>
size_t Histogram<T, Limits>::total() {
    HistogramBinSampleAdder<T, Limits> a;
    return std::accumulate(begin(), end(), 0, a);
}

template <typename T, template <class> class Limits>
bool Histogram<T, Limits>::verify() {
    T prev = Limits<T>::min();
    int pos(0);
    for (const auto& bin : bins) {
        if (bin->start() != prev) {
            std::cerr << "Expected " << bin->start() << " == " << prev
                      << " at pos " << pos << std::endl;
            return false;
        }
        if (bin->start() != prev) {
            return false;
        }
        prev = bin->end();
        ++pos;
    }
    if (prev != Limits<T>::max()) {
        return false;
    }
    return true;
}

template <typename T, template <class> class Limits>
typename Histogram<T, Limits>::iterator Histogram<T, Limits>::findBin(
        T amount) {
    if (amount == Limits<T>::max()) {
        return bins.end() - 1;
    } else {
        iterator it;
        it = std::upper_bound(bins.begin(),
                              bins.end(),
                              amount,
                              [](T t, Histogram<T, Limits>::value_type& b) {
                                  return t < b->end();
                              });
        if (!(*it)->accepts(amount)) {
            return bins.end();
        }
        return it;
    }
}

// Explicit template instantiations for all classes which we specialise
// Histogram<> for.
template class PLATFORM_PUBLIC_API Histogram<uint16_t>;
template class PLATFORM_PUBLIC_API Histogram<uint32_t>;
template class PLATFORM_PUBLIC_API Histogram<size_t>;
template class PLATFORM_PUBLIC_API Histogram<int>;
template class PLATFORM_PUBLIC_API Histogram<UnsignedMicroseconds, cb::duration_limits>;

// This is non-inline because it requires the definition of histogram and hence
// if inline would trigger an implicit instantiation of MicrosecondHistogram,
// which causes linker errors for any target which includes histogram.h and not
// histogram.cc
void MicrosecondStopwatch::stop(ProcessClock::time_point end) {
    const auto spent = end - startTime;
    histogram.add(std::chrono::duration_cast<std::chrono::microseconds>(spent));
}

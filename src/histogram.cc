/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
size_t Histogram<T, Limits>::total() const {
    HistogramBinSampleAdder<T, Limits> a;
    return std::accumulate(begin(), end(), size_t(0), a);
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
template class Histogram<uint16_t>;
template class Histogram<uint32_t>;
template class Histogram<size_t>;
template class Histogram<int>;
template class Histogram<UnsignedMicroseconds, cb::duration_limits>;

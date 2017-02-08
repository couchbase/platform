/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <inttypes.h>
#include <iterator>
#include <limits>
#include <numeric>
#include <platform/platform.h>
#include <platform/make_unique.h>
#include <relaxed_atomic.h>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <vector>

// Forward declaration.
template<typename T>
class Histogram;

/**
 * An individual bin of histogram data.
 */
template<typename T>
class HistogramBin {
public:

    /**
     * Get a HistogramBin covering [s, e)
     */
    HistogramBin(T s, T e)
        : _count(0),
          _start(s),
          _end(e) { }

    HistogramBin(const HistogramBin& other) = delete;

    /**
     * The starting value of this histogram bin (inclusive).
     */
    T start() const { return _start; }

    /**
     * The ending value of this histogram bin (exclusive).
     */
    T end() const { return _end; }

    /**
     * The count in this bin.
     */
    size_t count() const { return _count.load(); }

private:
    friend class Histogram<T>;

    /**
     * Increment this bin by the given amount.
     */
    void incr(size_t amount) {
        _count.fetch_add(amount);
    }

    /**
     * Set a specific value for this bin.
     */
    void set(size_t val) {
        _count.store(val);
    }

    /**
     * Does this bin contain the given value?
     *
     * @param value a value that may be within this bin's boundaries
     * @return true if this value is counted within this bin
     */
    bool accepts(T value) {
        return value >= _start &&
               (value < _end || value == std::numeric_limits<T>::max());
    }

    Couchbase::RelaxedAtomic<size_t> _count;
    T _start;
    T _end;
};

/**
 * Helper function object to sum sample.
 */
template<typename T>
class HistogramBinSampleAdder {
public:
    HistogramBinSampleAdder() { }

    size_t operator()(size_t n, const typename Histogram<T>::value_type& b) {
        return n + b->count();
    }
};

/**
 * A bin generator that generates buckets of a width that may increase
 * in size a fixed growth amount over iterations.
 */
template<typename T>
class GrowingWidthGenerator {
public:

    /**
     * Construct a growing width generator.
     *
     * @param start the starting point for this generator (inclusive)
     * @param width the starting width for this generator
     * @param growth how far the width should increase each time
     */
    GrowingWidthGenerator(T start, T width, double growth = 1.0)
        : _growth(growth),
          _start(start),
          _width(static_cast<double>(width)) { }

    /**
     * Generate the next bin.
     */
    typename Histogram<T>::value_type operator()() {
        typename Histogram<T>::value_type rv =
                std::make_unique<typename Histogram<T>::bin_type>
                        (_start,
                         _start + static_cast<T>(_width));
        _start += static_cast<T>(_width);
        _width = _width * _growth;
        return rv;
    }

private:
    double _growth;
    T _start;
    double _width;
};

/**
 * A bin generator that generates buckets from a sequence of T where
 * each bin is from [v[n], v[n+1]).
 */
template<typename T>
class FixedInputGenerator {
public:

    /**
     * Get a FixedInputGenerator with the given sequence of bin starts.
     */
    FixedInputGenerator(std::vector<T>& input)
        : it(input.begin()),
          end(input.end()) { }

    typename Histogram<T>::value_type operator()() {
        if (it + 1 >= end) {
            throw std::overflow_error("FixedInputGenerator::operator()"
                                              "would overflow input sequence");
        }
        T current = *it;
        ++it;
        T next = *it;
        return std::make_unique<typename Histogram<T>::bin_type>(current,
                                                                 next);
    }

private:
    typename std::vector<T>::iterator it;
    const typename std::vector<T>::iterator end;
};

/**
 * A bin generator that [n^i, n^(i+1)).
 */
template<typename T>
class ExponentialGenerator {
public:

    /**
     * Get a FixedInputGenerator with the given sequence of bin starts.
     */
    ExponentialGenerator(uint64_t start, double power)
        : _start(start),
          _power(power) { }

    typename Histogram<T>::value_type operator()() {
        T start = static_cast<T>(std::pow(_power, static_cast<double>(_start)));
        T end = static_cast<T>(std::pow(_power, static_cast<double>(++_start)));
        return std::make_unique<typename Histogram<T>::bin_type>(start, end);
    }

private:
    uint64_t _start;
    double _power;
};

/**
 * A Histogram.
 */
template<typename T>
class Histogram {
public:

    using bin_type = HistogramBin<T>;
    using value_type = std::unique_ptr<bin_type>;
    using container_type = std::vector<value_type>;
    using const_iterator = typename container_type::const_iterator;
    using iterator = typename container_type::const_iterator;

    /**
     * Build a histogram.
     *
     * @param generator a generator for the bins within this bucket
     * @param n how many bins this histogram should contain
     */
    template<typename G>
    Histogram(const G& generator, size_t n = 30)
        : bins(n) {
        if (n < 1){
            throw std::invalid_argument("Histogram must have at least 1 bin");
        }
        fill(generator);
    }

    /**
     * Build a default histogram.
     *
     * @param n how many bins this histogram should contain.
     */
    Histogram(size_t n = 30)
        : Histogram(ExponentialGenerator<T>(0, 2.0), n) {
    }

    //Deleted to avoid copying large objects
    Histogram(const Histogram& other) = delete;

    //Can't default this due to MSVC 2013
    Histogram(Histogram&& other)
        : bins(std::move(other.bins)) {
    }

    Histogram& operator=(Histogram&& other){
        bins = other.bins;
        return *this;
    }

    Histogram& operator=(const Histogram& other) = delete;

    /**
     * Add a value to this histogram.
     *
     * @param amount the size of the thing being added
     * @param count the quantity at this size being added
     */
    void add(T amount, size_t count = 1) {
        (*findBin(amount))->incr(count);
    }

    /**
     * Get the bin servicing the given sized input.
     */
    const HistogramBin<T>* getBin(T amount) {
        return findBin(amount)->get();
    }

    /**
     * Set all bins to 0.
     */
    void reset() {
        std::for_each(bins.begin(), bins.end(),
                      [](value_type& val){val->set(0);});
    }

    /**
     * Get the total number of samples counted.
     *
     * This is the sum of all counts in each bin.
     */
    size_t total() {
        HistogramBinSampleAdder<T> a;
        return std::accumulate(begin(), end(), 0, a);
    }



    /**
     * Get an iterator from the beginning of a histogram bin.
     */
    const_iterator begin() const {
        return bins.begin();
    }

    iterator begin() {
        return bins.begin();
    }

    /**
     * Get the iterator at the end of the histogram bin.
     */
    const_iterator end() const {
        return bins.end();
    }

    iterator end() {
        return bins.end();
    }

private:

    template<typename G>
    void fill(G& generator) {
        std::generate(bins.begin(), bins.end(), generator);

        // If there will not naturally be one, create a bin for the
        // smallest possible value
        if (bins.front()->start() > std::numeric_limits<T>::min()) {
            bins.insert(bins.begin(),
                        std::make_unique<bin_type>
                                (std::numeric_limits<T>::min(),
                                            bins.front()->start()));
        }

        // Also create one reaching to the largest possible value
        if (bins.back()->end() < std::numeric_limits<T>::max()) {
            bins.push_back(std::make_unique<bin_type>(bins.back()->end(),
                                               std::numeric_limits<T>::max()));
        }

        verify();
    }

    // This validates that we're sorted and have no gaps or overlaps. Returns
    // true if tests pass, else false.
    bool verify() {
        T prev = std::numeric_limits<T>::min();
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
        if (prev != std::numeric_limits<T>::max()) {
            return false;
        }
        return true;
    }

    /* Finds the bin containing the specified amount. Returns iterator to end
     * () if not found
     */
    iterator findBin(T amount) {
        if (amount == std::numeric_limits<T>::max()) {
            return bins.end() - 1;
        } else {
            iterator it;
            it = std::upper_bound(bins.begin(), bins.end(), amount,
                                  [](T t, Histogram<T>::value_type & b) {
                                      return t < b->end();
                                  });
            if (!(*it)->accepts(amount)) {
                return bins.end();
            }
            return it;
        }
    }

    template<typename Ttype>
    friend std::ostream& operator<<(std::ostream& out,
                                    const Histogram<Ttype>& b);

    container_type bins;
};



/**
 * Times blocks automatically and records the values in a histogram.
 *
 * If THRESHOLD_MS is greater than zero, then any blocks taking longer than
 * THRESHOLD_MS to execute will be reported to stderr.
 * Note this requires that a name is specified for the BlockTimer.
 */
template<typename HISTOGRAM,
    uint64_t THRESHOLD_MS>
class GenericBlockTimer {
public:

    /**
     * Get a BlockTimer that will store its values in the given
     * histogram.
     *
     * @param d the histogram that will hold the result
     * @param n the name to give the histogram, used when logging slow block
     *        execution to the log.
     */
    GenericBlockTimer(HISTOGRAM* d, const char* n = nullptr,
                      std::ostream* o = nullptr)
        : dest(d),
          start(gethrtime()),
          name(n),
          out(o) { }

    ~GenericBlockTimer() {
        hrtime_t spent(gethrtime() - start);
        dest->add(spent / 1000);
        log(spent, name, out);
    }

    static void log(hrtime_t spent, const char* name, std::ostream* o) {
        if (o && name) {
            *o << name << "\t" << spent << "\n";
        }
        if (THRESHOLD_MS > 0) {
            const uint64_t msec = spent / 1000000;
            if (name != nullptr && msec > THRESHOLD_MS) {
                std::cerr << "BlockTimer<" << name << "> Took too long: "
                          << msec << "ms" << std::endl;
            }
        }
    }

private:
    HISTOGRAM* dest;
    hrtime_t start;
    const char* name;
    std::ostream* out;
};

/* Convenience specialization which only records in a Histogram<hrtime_t>;
 * doesn't log slow blocks.
 */
typedef GenericBlockTimer<Histogram<hrtime_t>, 0> BlockTimer;

// How to print a bin.
template<typename T>
std::ostream& operator<<(std::ostream& out, const HistogramBin<T>& b) {
    out << "[" << b.start() << ", " << b.end() << ") = " << b.count();
    return out;
}

// How to print a histogram.
template<typename T>
std::ostream& operator<<(std::ostream& out, const Histogram<T>& b) {
    out << "{Histogram: ";
    bool needComma(false);
    for (const auto& bin : b.bins) {
        if (needComma) {
            out << ", ";
        }
        out << (*bin);
        needComma = true;
    }
    out << "}";
    return out;
}

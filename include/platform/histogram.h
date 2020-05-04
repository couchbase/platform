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

#include <platform/visibility.h>
#include <relaxed_atomic.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

// Custom microseconds duration used for measuring histogram stats.
// Stats will never be negative, so an unsigned Rep is used.
using UnsignedMicroseconds = std::chrono::duration<uint64_t, std::micro>;

namespace cb {
// std::chrono::durations are not supported by std::numeric_limits by default.
// This struct acts as a traits class for durations. It should be specialised
// for individual durations and implement a required subset of methods from
// std::numeric_limits
template <typename T>
struct duration_limits {};

template <>
struct duration_limits<UnsignedMicroseconds> {
    static UnsignedMicroseconds max() {
        return UnsignedMicroseconds::max();
    }
    static UnsignedMicroseconds min() {
        return UnsignedMicroseconds::min();
    }
};
}

// Forward declaration.
template <typename T, template <class> class Limits>
class Histogram;

/**
 * An individual bin of histogram data.
 *
 * @tparam T type of the histogram bin value.
 * @tparam Limits traits class which provides min() and max() static
 *         member functions which specify the min and max value of T.
 */
template <typename T, template <class> class Limits = std::numeric_limits>
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
    [[nodiscard]] T start() const {
        return _start;
    }

    /**
     * The ending value of this histogram bin (exclusive).
     */
    [[nodiscard]] T end() const {
        return _end;
    }

    /**
     * The count in this bin.
     */
    [[nodiscard]] size_t count() const {
        return _count.load();
    }

private:
    friend class Histogram<T, Limits>;

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
        return value >= _start && (value < _end || value == Limits<T>::max());
    }

    cb::RelaxedAtomic<size_t> _count;
    T _start;
    T _end;
};

/**
 * Helper function object to sum sample.
 */
template <typename T, template <class> class Limits>
class HistogramBinSampleAdder {
public:
    HistogramBinSampleAdder() { }

    size_t operator()(size_t n,
                      const typename Histogram<T, Limits>::value_type& b) {
        return n + b->count();
    }
};

/**
 * A bin generator that generates buckets of a width that may increase
 * in size a fixed growth amount over iterations.
 */
template <typename T, template <class> class Limits = std::numeric_limits>
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
          _start(start) {
        // Dividing two durations returns a value of the underlying Rep.
        _width = width / T(1);
    }

    /**
     * Generate the next bin.
     */
    typename Histogram<T, Limits>::value_type operator()() {
        auto rv = std::make_unique<typename Histogram<T, Limits>::bin_type>(
                _start, _start + static_cast<T>(uint64_t(_width)));
        _start += static_cast<T>(uint64_t(_width));
        _width = _width * _growth;
        return rv;
    }

private:
    double _growth;
    T _start;
    double _width;
};

/**
 * A bin generator that [n^i, n^(i+1)).
 */
template <typename T, template <class> class Limits = std::numeric_limits>
class ExponentialGenerator {
public:

    /**
     * Get a FixedInputGenerator with the given sequence of bin starts.
     */
    ExponentialGenerator(uint64_t start, double power)
        : _start(start),
          _power(power) { }

    typename Histogram<T, Limits>::value_type operator()() {
        T start = T(uint64_t(std::pow(_power, double(_start))));
        T end = T(uint64_t(std::pow(_power, double(++_start))));
        return std::make_unique<typename Histogram<T, Limits>::bin_type>(start,
                                                                         end);
    }

private:
    uint64_t _start;
    double _power;
};

// UnsignedMicroseconds stream operator required for Histogram::verify()
// so the type can be printed.
inline std::ostream& operator<<(std::ostream& os,
                                const UnsignedMicroseconds& ms) {
    os << ms.count();
    return os;
}

/**
 * A Histogram.
 */
template <typename T, template <class> class Limits = std::numeric_limits>
class Histogram {
public:
    using bin_type = HistogramBin<T, Limits>;
    using value_type = std::unique_ptr<bin_type>;
    using container_type = std::vector<value_type>;
    using const_iterator = typename container_type::const_iterator;
    using iterator = typename container_type::const_iterator;

    static constexpr size_t defaultNumBuckets = 30;

    /**
     * Build a histogram with a default number of buckets (30).
     *
     * @param generator a generator for the bins within this bucket
     */
    template <typename G>
    explicit Histogram(const G& generator)
        : Histogram(generator, defaultNumBuckets) {
    }

    /**
     * Build a histogram.
     *
     * @param generator a generator for the bins within this bucket
     * @param n how many bins this histogram should contain
     */
    template <typename G>
    Histogram(const G& generator, size_t n) : bins(n) {
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
    explicit Histogram(size_t n = defaultNumBuckets)
        : Histogram(ExponentialGenerator<T, Limits>(0, 2.0), n) {
    }

    //Deleted to avoid copying large objects
    Histogram(const Histogram& other) = delete;

    Histogram(Histogram&& other) = default;

    Histogram& operator=(Histogram&& other) = default;

    Histogram& operator=(const Histogram& other) = delete;

    /**
     * Add a value to this histogram.
     *
     * Note: defined non-inline to reduce the code bloat from recording
     *       histogram values - the inline size of this method is non-trivial.
     *       If you get want to add a new instantiation of this class; check
     *       the explicit template definition in histogram.cc.
     *
     * @param amount the size of the thing being added
     * @param count the quantity at this size being added
     */
    void add(T amount, size_t count = 1);

    /**
     * Get the bin servicing the given sized input.
     */
    const HistogramBin<T, Limits>* getBin(T amount) {
        return findBin(amount)->get();
    }

    /**
     * Set all bins to 0.
     */
    void reset();

    /**
     * Get the total number of samples counted.
     *
     * This is the sum of all counts in each bin.
     */
    size_t total() const;

    /**
     * Get an iterator from the beginning of a histogram bin.
     */
    [[nodiscard]] const_iterator begin() const {
        return bins.begin();
    }

    iterator begin() {
        return bins.begin();
    }

    /**
     * Get the iterator at the end of the histogram bin.
     */
    [[nodiscard]] const_iterator end() const {
        return bins.end();
    }

    iterator end() {
        return bins.end();
    }

    [[nodiscard]] size_t size() const {
        return bins.size();
    }

    [[nodiscard]] size_t getMemFootPrint() const {
        return sizeof(Histogram) + ((sizeof(bin_type) +
                sizeof(value_type)) * bins.size());
    }

private:

    template<typename G>
    void fill(G& generator) {
        std::generate(bins.begin(), bins.end(), generator);

        // If there will not naturally be one, create a bin for the
        // smallest possible value
        if (bins.front()->start() > Limits<T>::min()) {
            bins.insert(bins.begin(),
                        std::make_unique<bin_type>(Limits<T>::min(),
                                                   bins.front()->start()));
        }

        // Also create one reaching to the largest possible value
        if (bins.back()->end() < Limits<T>::max()) {
            bins.push_back(std::make_unique<bin_type>(bins.back()->end(),
                                                      Limits<T>::max()));
        }

        verify();
    }

    // This validates that we're sorted and have no gaps or overlaps. Returns
    // true if tests pass, else false.
    bool verify();

    /* Finds the bin containing the specified amount. Returns iterator to end
     * () if not found
     */
    iterator findBin(T amount);

    template <typename type, template <class> class limits>
    friend std::ostream& operator<<(std::ostream& out,
                                    const Histogram<type, limits>& b);

    container_type bins;
};

/**
 * Histogram of durations measured in microseconds.
 *
 * Typesafe; calling add() with any chrono::duration specialization (seconds,
 * milliseconds etc) will perform any necessary conversion.
 */
using MicrosecondHistogram =
        Histogram<UnsignedMicroseconds, cb::duration_limits>;

/**
 * Adapter class template to assist in recording the duration of an event into
 * Histogram T that has an add method T::add(std::chrono::microseconds v);
 * which don't have start() / stop() methods.
 *
 * This class will record the startTime when start() is called; then when end()
 * is called it will calculate the duration and pass that to Histogram.
 */
 template<typename T>
class MicrosecondStopwatch {
public:
    MicrosecondStopwatch(T& histogram)
        : histogram(histogram) {
    }

    void start(std::chrono::steady_clock::time_point start_) {
        startTime = start_;
    }

    void stop(std::chrono::steady_clock::time_point end) {
        const auto spent = end - startTime;
        histogram.add(std::chrono::duration_cast<std::chrono::microseconds>(spent));
    }

private:
    T& histogram;
    std::chrono::steady_clock::time_point startTime;
};

/**
 * Times blocks automatically and records the values in a histogram.
 *
 * If THRESHOLD_MS is greater than zero, then any blocks taking longer than
 * THRESHOLD_MS to execute will be reported to stderr.
 * Note this requires that a name is specified for the BlockTimer.
 */
template<typename HISTOGRAM, uint64_t THRESHOLD_MS>
class GenericBlockTimer {
public:

    /**
     * Get a BlockTimer that will store its values in the given
     * histogram.
     *
     * @param d the histogram that will hold the result.
     *        If it is 'nullptr', the BlockTimer is disabled so that
     *        both constructor and destructor will do nothing.
     * @param n the name to give the histogram, used when logging slow block
     *        execution to the log.
     */
    GenericBlockTimer(HISTOGRAM* d,
                      const char* n = nullptr,
                      std::ostream* o = nullptr)
        : dest(d),
          start((dest) ? std::chrono::steady_clock::now()
                       : std::chrono::steady_clock::time_point()),
          name(n),
          out(o) {
    }

    ~GenericBlockTimer() {
        if (dest) {
            auto spent = std::chrono::steady_clock::now() - start;
            dest->add(std::chrono::duration_cast<std::chrono::microseconds>(spent));
            log(spent, name, out);
        }
    }

    static void log(std::chrono::steady_clock::duration spent,
                    const char* name,
                    std::ostream* o) {
        if (o && name) {
            *o << name << "\t" << spent.count() << "\n";
        }
        if (THRESHOLD_MS > 0) {
            const auto msec =
                    std::make_unsigned<std::chrono::steady_clock::rep>::type(
                            std::chrono::duration_cast<
                                    std::chrono::milliseconds>(spent)
                                    .count());
            if (name != nullptr && msec > THRESHOLD_MS) {
                std::cerr << "BlockTimer<" << name
                          << "> Took too long: " << msec << "ms" << std::endl;
            }
        }
    }

private:
    HISTOGRAM* dest;
    std::chrono::steady_clock::time_point start;
    const char* name;
    std::ostream* out;
};

/* Convenience specialization which only records in a MicrosecondHistogram;
 * doesn't log slow blocks.
 */
typedef GenericBlockTimer<MicrosecondHistogram, 0> BlockTimer;

// How to print a bin.
template <typename T, template <class> class Limits>
std::ostream& operator<<(std::ostream& out, const HistogramBin<T, Limits>& b) {
    out << "[" << b.start() << ", " << b.end() << ") = " << b.count();
    return out;
}

// How to print a histogram.
template <typename T, template <class> class Limits>
std::ostream& operator<<(std::ostream& out, const Histogram<T, Limits>& b) {
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

/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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

#include "config.h"

#include <platform/processclock.h>
#include <atomic>

namespace cb {
/* Wrapper class around std::atomic. It provides atomic operations
 * for std::chrono durations by operating on the underlying value
 * obtained from the duration's count().
 * It uses relaxed memory ordering, so it is suitable for statistics use only.
 */
class AtomicDuration {
public:
    AtomicDuration() {
        store(ProcessClock::duration::zero());
    }

    AtomicDuration(ProcessClock::duration initial) {
        store(initial);
    }

    explicit AtomicDuration(const AtomicDuration& other) {
        store(other.load());
    }

    operator ProcessClock::duration() const {
        return load();
    }

    ProcessClock::duration load() const {
        return ProcessClock::duration(value.load(std::memory_order_relaxed));
    }

    void store(ProcessClock::duration desired) {
        value.store(desired.count(), std::memory_order_relaxed);
    }

    ProcessClock::duration fetch_add(ProcessClock::duration arg) {
        return ProcessClock::duration(
                value.fetch_add(arg.count(), std::memory_order_relaxed));
    }

    ProcessClock::duration fetch_sub(ProcessClock::duration arg) {
        return ProcessClock::duration(
                value.fetch_sub(arg.count(), std::memory_order_relaxed));
    }

    AtomicDuration& operator=(ProcessClock::duration val) {
        store(val);
        return *this;
    }

    AtomicDuration& operator+=(ProcessClock::duration rhs) {
        fetch_add(rhs);
        return *this;
    }

    AtomicDuration& operator-=(ProcessClock::duration rhs) {
        fetch_sub(rhs);
        return *this;
    }

    ProcessClock::duration operator++() {
        return fetch_add(ProcessClock::duration(1)) + ProcessClock::duration(1);
    }

    ProcessClock::duration operator++(int) {
        return fetch_add(ProcessClock::duration(1));
    }

    ProcessClock::duration operator--() {
        return fetch_sub(ProcessClock::duration(1)) - ProcessClock::duration(1);
    }

    ProcessClock::duration operator--(int) {
        return fetch_sub(ProcessClock::duration(1));
    }

private:
    std::atomic<ProcessClock::duration::rep> value;
};
}
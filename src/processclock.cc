/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/processclock.h>

std::chrono::steady_clock::time_point cb::DefaultProcessClockSource::now() {
    return std::chrono::steady_clock::now();
}

static cb::DefaultProcessClockSource clockSource;

cb::DefaultProcessClockSource& cb::defaultProcessClockSource() {
    return clockSource;
}

std::chrono::nanoseconds cb::to_ns_since_epoch(
        const std::chrono::steady_clock::time_point& tp) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                         tp.time_since_epoch());
}
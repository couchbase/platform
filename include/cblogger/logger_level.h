/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

namespace cb::logger {

/*
0 -> SPDLOG_LEVEL_TRACE
1 -> SPDLOG_LEVEL_DEBUG
2 -> SPDLOG_LEVEL_INFO
3 -> SPDLOG_LEVEL_WARN
4 -> SPDLOG_LEVEL_ERROR
5 -> SPDLOG_LEVEL_CRITICAL
6 -> SPDLOG_LEVEL_OFF
*/
enum class Level : int {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    err = 4,
    critical = 5,
    off = 6,
};
} // namespace cb::logger

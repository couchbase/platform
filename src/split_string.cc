/*
 *    Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/split_string.h>

std::vector<std::string_view> cb::string::split(std::string_view s,
                                                char delim,
                                                bool allowEmpty) {
    std::vector<std::string_view> result;
    while (true) {
        const auto m = s.find(delim);
        if (m == std::string_view::npos) {
            break;
        }

        result.emplace_back(s.data(), m);
        s.remove_prefix(m + 1);
        if (!allowEmpty) {
            while (s.front() == delim) {
                s.remove_prefix(1);
            }
        }
    }
    if (!s.empty()) {
        result.emplace_back(s);
    }
    return result;
}

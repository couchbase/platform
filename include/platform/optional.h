/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <optional>
#include <string>

/**
 * Returns a string representation of `val` if the optional is non-empty using
 * to_string(T), otherwise returns "none".
 */
template <typename T>
std::string to_string_or_none(std::optional<T> val) {
    using namespace std;
    if (val) {
        return to_string(*val);
    }
    return "none";
}

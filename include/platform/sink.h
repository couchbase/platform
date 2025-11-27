/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <cstddef>
#include <string_view>
namespace cb::io {

// Abstract Sink for moving data from network to a destination
class Sink {
public:
    virtual ~Sink() = default;
    virtual void sink(std::string_view data) = 0;
    virtual std::size_t fsync() = 0;
    virtual std::size_t close() = 0;
    virtual std::size_t getBytesWritten() const = 0;
};

} // namespace cb::io
/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/uuid.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

cb::uuid::uuid_t cb::uuid::random() {
    return boost::uuids::random_generator()();
}

cb::uuid::uuid_t cb::uuid::from_string(std::string_view str) {
    try {
        return boost::lexical_cast<uuid_t>(str);
    } catch (const boost::exception&) {
        // backwards compat to make sure we don't crash for different reasons
        throw std::invalid_argument(
                "cb::uuid::from_string: Failed to convert string");
    }
}

std::string to_string(const cb::uuid::uuid_t& uuid) {
    return boost::uuids::to_string(uuid);
}

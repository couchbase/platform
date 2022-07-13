/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <boost/uuid/uuid.hpp>

namespace cb::uuid {

using uuid_t = boost::uuids::uuid;

/**
 * Generate a new random uuid and return it
 */
uuid_t random();

/**
 * Convert a textual version of a UUID to a uuid type
 * @throw std::invalid_argument if the textual uuid is not
 *        formatted correctly
 */
uuid_t from_string(std::string_view str);
} // namespace cb::uuid

/**
 * Print a textual version of the UUID in the form:
 *
 *     00000000-0000-0000-0000-000000000000
 */
std::string to_string(const cb::uuid::uuid_t& uuid);

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

#include <platform/string_hex.h>
#include <iomanip>
#include <random>
#include <sstream>

void cb::uuid::random(cb::uuid::uuid_t& uuid) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    // The uuid is 16 bytes, which is the same as two 64 bit integers
    auto* ptr = reinterpret_cast<uint64_t*>(uuid.data());
    ptr[0] = dis(gen);
    ptr[1] = dis(gen);

    // Make sure that it looks like a version 4
    uuid[6] &= 0x0f;
    uuid[6] |= 0x40;
}

cb::uuid::uuid_t cb::uuid::random() {
    uuid_t ret;
    random(ret);
    return ret;
}

cb::uuid::uuid_t cb::uuid::from_string(std::string_view str) {
    uuid_t ret;
    if (str.size() != 36) {
        throw std::invalid_argument(
                "cb::uuid::from_string: string was wrong size got: " +
                std::to_string(str.size()) + " (expected: 36)");
    }

    int jj = 0;
    for (int ii = 0; ii < 36; ii += 2) {
        switch (ii) {
        case 8:
        case 13:
        case 18:
        case 23:
            if (str[ii] != '-') {
                throw std::invalid_argument(
                        "cb::uuid::from_string: hyphen not found where "
                        "expected");
            }
            ++ii;
        default:
            ret[jj++] = uint8_t(cb::from_hex({str.data() + ii, 2}));
        }
    }
    return ret;
}

std::string to_string(const cb::uuid::uuid_t& uuid) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    int ii = 0;
    for (const auto& c : uuid) {
        ss << std::setw(2) << uint32_t(c);
        switch (++ii) {
        case 4:
        case 6:
        case 8:
        case 10:
            ss << '-';
        default:;
        }
    }

    return ss.str();
}

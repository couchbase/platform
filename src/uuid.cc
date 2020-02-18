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
#include <platform/uuid.h>

#include <platform/string_hex.h>
#include <iomanip>
#include <random>
#include <sstream>

PLATFORM_PUBLIC_API
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

PLATFORM_PUBLIC_API
cb::uuid::uuid_t cb::uuid::random() {
    uuid_t ret;
    random(ret);
    return ret;
}

PLATFORM_PUBLIC_API
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

PLATFORM_PUBLIC_API
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

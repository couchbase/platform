/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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

#include "config.h"

#include <limits>
#include <extmeta/extmeta.h>
#include <stdexcept>
#include <string>

cb::extmeta::Builder::Builder()
    : encoded(false) {
    data.push_back(uint8_t(Version::ONE));
}

namespace cb {
namespace extmeta {
class Receiver : public cb::extmeta::Parser::Receiver {
public:
    Receiver(Builder& b)
        : builder(b) {}

    bool add(const Type& type, const uint8_t* data, size_t size) override {
        builder.add(type, data, size);
        return true;
    }

protected:
    Builder& builder;
};
}
}

cb::extmeta::Builder::Builder(const uint8_t* meta, uint16_t length)
    : Builder() {
    Receiver receiver(*this);
    Parser::parse(meta, length, receiver);
}

cb::extmeta::Builder::Builder(const std::vector<uint8_t>& meta)
    : Builder(meta.data(), static_cast<uint16_t>(meta.size())) {
    // empty
}

cb::extmeta::Builder& cb::extmeta::Builder::add(const Type& type,
                                                const uint8_t* value,
                                                size_t length) {
    if (encoded) {
        throw std::logic_error(
            "cb::extmeta::Builder::add: Can't add on a encoded buffer");
    }

    if (uint8_t(type) >= uint8_t(0x80)) {
        throw std::invalid_argument(
            "cb::extmeta::Builder::add: types in the range [0x80-0xff] are reserved for future use");
    }

    for (const auto& t : types) {
        if (t == type) {
            throw std::logic_error(
                "cb::extmeta::Builder::add: each type can be added only once");
        }
    }

    types.push_back(type);
    data.push_back(uint8_t(type));
    uint16_t len = htons(uint16_t(length));
    size_t offset = data.size();
    data.resize(offset + 2 + length);

    uint8_t* ptr = data.data() + offset;
    memcpy(ptr, &len, 2);
    memcpy(ptr + 2, value, length);

    if (data.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::overflow_error(
            "cb::extmeta::Builder::add: meta exceeds 64k: "
            + std::to_string(offset) + " > "
            + std::to_string(
                std::numeric_limits<uint16_t>::max()));
    }

    return *this;
}

cb::extmeta::Builder& cb::extmeta::Builder::add(const Type& type,
                                                const std::string& value) {
    return add(type, reinterpret_cast<const uint8_t*>(value.data()),
               value.size());
}

cb::extmeta::Builder& cb::extmeta::Builder::encode() {
    if (encoded) {
        throw std::logic_error(
            "cb::extmeta::Builder::encode: should only be called once");
    }

    encoded = true;
    // There is no use of using an empty meta block
    if (data.size() == 1) {
        data.resize(0);
    }

    return *this;
}

const std::vector<uint8_t>& cb::extmeta::Builder::getEncodedData() const {
    if (!encoded) {
        throw std::logic_error(
            "cb::extmeta::Builder::getEncodedData: encode() not called");
    }
    return data;
}

uint16_t cb::extmeta::Builder::to_network(uint16_t val) {
    return ntohs(val);
}

uint32_t cb::extmeta::Builder::to_network(uint32_t val) {
    return ntohl(val);
}

uint64_t cb::extmeta::Builder::to_network(uint64_t val) {
    return ntohll(val);
}

void cb::extmeta::Parser::parse(const uint8_t* meta, uint16_t nmeta,
                                Receiver& receiver) {
    if (nmeta == 0) {
        return;
    }
    if (Version(meta[0]) != Version::ONE) {
        throw std::invalid_argument(
            "cb::extmeta::Parser: Unknown version " +
            std::to_string(uint8_t(meta[0])));
    }

    size_t offset = 1;

    while (offset < nmeta) {
        // First we have the type
        uint8_t type = meta[offset++];
        // next follows 2 byte of length
        if ((offset + 2) > nmeta) {
            throw std::underflow_error("cb::extmeta::Parser: premature EOF");
        }
        uint16_t len;
        memcpy(&len, meta + offset, sizeof(len));
        len = ntohs(len);
        offset += 2;

        // next follows len bytes of value
        if (offset + len > nmeta) {
            throw std::underflow_error("cb::extmeta::Parser: premature EOF");
        }

        if (!receiver.add(Type(type), meta + offset, len)) {
            break;
        }
        offset += len;
    }
}

void cb::extmeta::Parser::parse(const std::vector<uint8_t>& meta,
                                Receiver& receiver) {
    parse(meta.data(), static_cast<uint16_t>(meta.size()), receiver);
}

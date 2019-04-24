/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc.
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

#include <nlohmann/json.hpp>
#include <platform/pipe.h>

#include <sstream>

namespace cb {
void Pipe::produced(size_t nbytes) {
    if (write_head + nbytes > buffer.size()) {
        std::stringstream ss;
        ss << "Pipe::produced(): Produced bytes exceeds the number of "
              "available bytes. { \"nbytes\": "
           << std::to_string(nbytes) << ","
           << " \"buffer.size()\": " << std::to_string(buffer.size()) << ","
           << " \"write_head\": " << std::to_string(write_head) << "}";
        throw std::logic_error(ss.str());
    }
    write_head += nbytes;
}

nlohmann::json Pipe::to_json() {
    nlohmann::json ret;
    ret["buffer"] = uintptr_t(buffer.data());
    ret["size"] = buffer.size();
    ret["read_head"] = read_head;
    ret["write_head"] = write_head;
    ret["empty"] = empty();

    return ret;
}
} // namespace cb
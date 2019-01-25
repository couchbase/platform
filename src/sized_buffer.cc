/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc
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

#include <platform/sized_buffer.h>
#include <ostream>

PLATFORM_PUBLIC_API
std::string cb::to_string(cb::const_char_buffer cb) {
    return std::string{cb.data(), cb.size()};
}

PLATFORM_PUBLIC_API
std::ostream& cb::operator<<(std::ostream& os,
                             const cb::const_char_buffer& cb) {
    return os.write(cb.data(), cb.size());
}

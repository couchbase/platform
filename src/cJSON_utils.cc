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
#include <cstddef>
#include <cJSON_utils.h>
#include <stdexcept>

CJSON_PUBLIC_API
std::string to_string(const cJSON* json, bool formatted) {
    if (json == nullptr) {
        throw std::invalid_argument("to_string(cJSON*): json pointer can't be"
                                        " null");
    }

    char* ptr;
    if (formatted) {
        ptr = cJSON_Print(const_cast<cJSON*>(json));
    } else {
        ptr = cJSON_PrintUnformatted(const_cast<cJSON*>(json));
    }

    if (ptr == nullptr) {
        throw std::bad_alloc();
    }

    std::string ret(ptr);
    cJSON_Free(ptr);

    return ret;
}

CJSON_PUBLIC_API
std::string to_string(const unique_cJSON_ptr& json, bool formatted) {
    return to_string(json.get(), formatted);
}

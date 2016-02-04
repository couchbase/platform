/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc.
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
#pragma once

#include <cstddef>
#include <cJSON.h>
#include <memory>
#include <string>

// helper class for use with std::unique_ptr in managing cJSON* objects.
struct cJSONDeleter {
    void operator()(cJSON* j) { cJSON_Delete(j); }
};

typedef std::unique_ptr<cJSON, cJSONDeleter> unique_cJSON_ptr;

/**
 * Convert a cJSON structure to a string representation.
 *
 * @param json pointer to the JSON representation of the data
 * @param formatted should the string representation be formatted or
 *                  compact.
 * @throws std::bad_alloc for memory allocation issues
 * @throws std::invalid_argument for invalid parameters
 */
CJSON_PUBLIC_API
std::string to_string(const cJSON* json, bool formatted = true);

CJSON_PUBLIC_API
std::string to_string(const unique_cJSON_ptr& json, bool formatted = true);

/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2021 Couchbase, Inc.
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
#include <platform/processcontrol.h>

#ifdef _WIN32
#include <process.h>
#define EXEC_FUNCTION _execv
// folly has a win32 setenv
#include <folly/portability/Stdlib.h>
#else
#include <unistd.h>
#define EXEC_FUNCTION execv
#endif

#include <exception>
#include <system_error>

namespace cb {

void execWithUpdatedEnvironment(int argc,
                                char** argv,
                                std::string_view var,
                                std::string_view value) {
    if (argc == 0) {
        throw std::invalid_argument("argc is 0");

    } else if (!argv[0]) {
        throw std::invalid_argument("argv[0] must not be null");
    }
    auto* current = getenv(var.data());
    bool updatedEnvironment = false;
    if (!current) {
        if (setenv(var.data(), value.data(), 0) == 0) {
            updatedEnvironment = true;
        } else {
            throw std::system_error(
                    errno, std::generic_category(), "setenv failed");
        }
    }

    // Only proceed to execv if the environment was changed
    if (updatedEnvironment && EXEC_FUNCTION(argv[0], argv)) {
        throw std::system_error(errno, std::generic_category(), "execv failed");
    }
}
} // namespace cb
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

#include <string_view>

namespace cb {

/**
 * Check the current environment for var, if var is not present in the
 * environment, set var=value and then call execv(argv[0], argv);
 *
 * @param argc length of argument array
 * @param argv argument array
 * @param var name on environment variable to check for
 * @param value value of environment variable to use if var is not found
 * @throws std::invalid_argument for bad input
 * @throws std::system_error for failure of setenv/execv
 */
void execWithUpdatedEnvironment(int argc,
                                char** argv,
                                std::string_view var,
                                std::string_view value);
} // namespace cb
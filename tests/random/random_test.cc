/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2014 Couchbase, Inc
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
#include <platform/random.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

using cb::RandomGenerator;
using std::cerr;
using std::endl;

static int basic_rand_tests(RandomGenerator& r1, RandomGenerator& r2) {
    uint64_t v1 = r1.next();
    uint64_t v2 = r2.next();
    if (v1 == v2) {
        cerr << "I did not expect the random generators to return the"
             << " same value" << endl;
        return -1;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    if (r1.getBytes(buffer, sizeof(buffer))) {
        size_t ii;

        for (ii = 0; ii < (size_t)sizeof(buffer); ++ii) {
            if (buffer[ii] != 0) {
                break;
            }
        }

        if (ii == (size_t)sizeof(buffer)) {
            cerr << "I got 1k of 0 (or it isn't working)" << endl;

            return -1;
        }
    }

    return 0;
}

int main() {
    auto r1 = std::make_unique<RandomGenerator>();
    auto r2 = std::make_unique<RandomGenerator>();

    if (basic_rand_tests(*r1, *r2) != 0) {
        return -1;
    }

    return 0;
}

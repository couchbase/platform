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

#include "platform-dirutils-test-library_export.h"

// I don't want to play around with name mangling
extern "C" {

PLATFORM_DIRUTILS_TEST_LIBRARY_EXPORT
int value = 5;

PLATFORM_DIRUTILS_TEST_LIBRARY_EXPORT
int getValue() {
    return value;
}

PLATFORM_DIRUTILS_TEST_LIBRARY_EXPORT
void setValue(int v) {
    value = v;
}
}
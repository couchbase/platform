/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2020 Couchbase, Inc
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
#include <platform/dirutils.h>

#ifdef WIN32
#include <folly/portability/Windows.h>
#include <thread>
#endif

#include <platform/memorymap.h>
#include <system_error>

DIRUTILS_PUBLIC_API
std::string cb::io::loadFile(const std::string& name) {
#ifdef WIN32
    // We've seen sporadic unit test failures on Windows due to sharing
    // errors (most likely caused by the other process is _creating_ the file
    // and still keeping it open). Given that loadFile shouldn't be called
    // from the front end threads (as it involves disk io which may be slow
    // in the first place) we'll try to back off a few times and retry
    // until we've figured out exactly what's causing the problem.
    int retrycount = 100;
    std::error_code code{};
    do {
        try {
            MemoryMappedFile map(name, MemoryMappedFile::Mode::RDONLY);
            return to_string(map.content());
        } catch (const std::system_error& error) {
            code = error.code();
            if (code.category() != std::system_category() ||
                code.value() != ERROR_SHARING_VIOLATION) {
                throw std::system_error(
                        code.value(),
                        code.category(),
                        "cb::io::loadFile(" + name + ") failed");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            --retrycount;
        }
    } while (retrycount > 0);

    throw std::system_error(code.value(),
                            code.category(),
                            "cb::io::loadFile(" + name + ") failed");
#else
    MemoryMappedFile map(name, MemoryMappedFile::Mode::RDONLY);
    return to_string(map.content());
#endif
}

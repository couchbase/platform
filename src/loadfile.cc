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
#else
#include <folly/FileUtil.h>
#endif

#include <cerrno>
#include <system_error>

#ifdef WIN32
// Ideally I would have wanted to use folly::readFile on Windows as well,
// but unfortunately it use the posix API on windows which don't properly
// set FILE_SHARE_WRITE causing us to fail to open the file if someone
// else have the file open for writing
std::string loadFileImpl(const std::string& name) {
    auto filehandle = CreateFile(name.c_str(),
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 nullptr);
    if (filehandle == INVALID_HANDLE_VALUE) {
        const auto error = GetLastError();
        throw std::system_error(
                error,
                std::system_category(),
                "loadFileImpl(): CreateFile(" + name + ") failed");
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesEx(name.c_str(), GetFileExInfoStandard, &fad) == 0) {
        auto error = GetLastError();
        CloseHandle(filehandle);
        throw std::system_error(
                error,
                std::system_category(),
                "loadFileImpl() GetFileAttributesEx(" + name + ") failed");
    }

    if (fad.nFileSizeHigh != 0) {
        CloseHandle(filehandle);
        throw std::runtime_error(
                "loadFileImpl(): File exceeds the maximum supported size");
    }

    std::string content;
    content.resize(fad.nFileSizeLow);
    DWORD nr;

    if (!ReadFile(filehandle,
                  const_cast<char*>(content.data()),
                  DWORD(content.size()),
                  &nr,
                  nullptr)) {
        auto error = GetLastError();
        CloseHandle(filehandle);
        throw std::system_error(
                error,
                std::system_category(),
                "loadFileImpl(): ReadFile(" + name + ") failed");
    }

    CloseHandle(filehandle);
    return content;
}
#else
static std::string loadFileImpl(const std::string& name) {
    std::string content;
    if (folly::readFile<std::string>(name.c_str(), content)) {
        return content;
    }
    throw std::system_error(errno,
                            std::system_category(),
                            "cb::io::loadFile(" + name + ") failed");
}
#endif

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
#endif
            return loadFileImpl(name);
#ifdef WIN32
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
#endif
}

/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/dirutils.h>

#ifdef WIN32
#include <folly/portability/Windows.h>
#else
#include <folly/FileUtil.h>
#endif

#include <platform/split_string.h>
#include <cerrno>
#include <filesystem>
#include <system_error>
#include <thread>

#ifdef WIN32
// Ideally I would have wanted to use folly::read_file on Windows as well,
// but unfortunately it use the posix API on windows which don't properly
// set FILE_SHARE_WRITE causing us to fail to open the file if someone
// else have the file open for writing
std::string loadFileImpl(const std::string& name, size_t bytesToRead) {
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
    size_t size = std::min(size_t(fad.nFileSizeLow), bytesToRead);
    try {
        content.resize(size);
    } catch (const std::bad_alloc&) {
        CloseHandle(filehandle);
        throw;
    }
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
static std::string loadFileImpl(const std::string& name, size_t bytesToRead) {
    std::string content;
    if (folly::readFile<std::string>(name.c_str(), content, bytesToRead)) {
        return content;
    }
    throw std::system_error(errno,
                            std::system_category(),
                            "cb::io::loadFile(" + name + ") failed");
}
#endif

std::string cb::io::loadFile(const std::filesystem::path& path,
                             std::chrono::microseconds waittime,
                             size_t bytesToRead) {
    const auto name = path.string();
#ifdef WIN32
    // We've seen sporadic unit test failures on Windows due to sharing
    // errors (most likely caused by the other process is _creating_ the file
    // and still keeping it open). Given that loadFile shouldn't be called
    // from the front end threads (as it involves disk io which may be slow
    // in the first place) we'll try to back off a few times and retry
    // until we've figured out exactly what's causing the problem.
    int retrycount = 100;
#else
    int retrycount = 0;
#endif

    const auto timeout = std::chrono::steady_clock::now() + waittime;
    do {
        std::string content;
        bool success = false;
        try {
            content = loadFileImpl(name, bytesToRead);
            success = true;
        } catch (const std::system_error& error) {
            const auto& code = error.code();
            if (code.category() != std::system_category()) {
                throw std::system_error(
                        code.value(),
                        code.category(),
                        "cb::io::loadFile(" + name + ") failed");
            }

            switch (code.value()) {
            case int(std::errc::no_such_file_or_directory):
                // we might want to retry
                break;
#ifdef WIN32
            case ERROR_SHARING_VIOLATION:
                --retrycount;
                break;
#endif
            default:
                throw std::system_error(
                        code.value(),
                        code.category(),
                        "cb::io::loadFile(" + name + ") failed");
            }
        }

        if (success) {
            return content;
        }
        if (waittime.count() != 0 || retrycount > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } while (std::chrono::steady_clock::now() < timeout || retrycount > 0);
#ifdef WIN32
    if (retrycount == 0) {
        throw std::system_error(
                ERROR_SHARING_VIOLATION,
                std::system_category(),
                "cb::io::loadFile(" + name + ") failed (with retry)");
    }
#endif
    if (waittime.count() > 0) {
        throw std::system_error(
                int(std::errc::no_such_file_or_directory),
                std::system_category(),
                "cb::io::loadFile(" + name + ") failed (with retry)");
    }
    throw std::system_error(int(std::errc::no_such_file_or_directory),
                            std::system_category(),
                            "cb::io::loadFile(" + name + ") failed");
}

/**
 * Read a file line by line and tokenize the line with the provided tokens.
 * If the callback returns false parsing of the file will stop.
 *
 * @param name The name of the file to parse
 * @param callback The callback provided by the user
 * @param delim The delimeter used to separate the fields in the file
 */
void cb::io::tokenizeFileLineByLine(
        const std::filesystem::path& name,
        std::function<bool(const std::vector<std::string_view>&)> callback,
        char delim,
        bool allowEmpty) {
    auto content = loadFile(name, std::chrono::microseconds{});
    auto lines = cb::string::split(content, '\n');
    for (auto line : lines) {
        while (line.back() == '\r') {
            line.remove_suffix(1);
        }
        if (!callback(cb::string::split(line, delim, allowEmpty))) {
            return;
        }
    }
}

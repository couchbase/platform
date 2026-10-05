/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/dirutils.h>

#ifdef WIN32

void cb::io::fsyncDirectory(const std::filesystem::path&) {
    // Windows doesn't support fsync on a directory handle (NTFS journals
    // metadata changes such as the rename)
}

#else
#include <fmt/format.h>

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <system_error>

void cb::io::fsyncDirectory(const std::filesystem::path& directory) {
    const auto dir = directory.empty() ? std::filesystem::path(".") : directory;
    const int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        throw std::system_error(
                errno,
                std::system_category(),
                fmt::format("Failed to open directory '{}'", dir.string()));
    }
    const int rv = ::fsync(fd);
    const int error = errno;
    ::close(fd);
    if (rv == -1) {
        throw std::system_error(
                error,
                std::system_category(),
                fmt::format("Failed to fsync directory '{}'", dir.string()));
    }
}
#endif

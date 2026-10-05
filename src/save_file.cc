/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <fmt/format.h>
#include <platform/dirutils.h>
#include <platform/file_sink.h>
#include <cerrno>
#include <cstdio>

namespace cb::io {
void saveFile(const std::filesystem::path& path,
              std::string_view content,
              std::ios_base::openmode mode) {
    // Use stdio rather than std::ofstream as the latter reports all errors
    // as std::io_errc::stream and the real cause (errno) is lost
    std::string fmode = (mode & std::ios_base::app) ? "a" : "w";
    if (mode & std::ios_base::binary) {
        fmode.push_back('b');
    }

    auto* fp = fopen(path.string().c_str(), fmode.c_str());
    if (fp == nullptr) {
        throw std::system_error(
                errno,
                std::generic_category(),
                fmt::format("Failed to open file '{}'", path.string()));
    }

    if (!content.empty() &&
        fwrite(content.data(), content.size(), 1, fp) != 1) {
        const auto error = errno;
        fclose(fp);
        throw std::system_error(
                error,
                std::generic_category(),
                fmt::format("Failed to write to file '{}'", path.string()));
    }

    if (fclose(fp) != 0) {
        throw std::system_error(
                errno,
                std::generic_category(),
                fmt::format("Failed to close file '{}'", path.string()));
    }
}

bool saveFile(const std::filesystem::path& path,
              std::string_view content,
              std::error_code& ec,
              std::ios_base::openmode mode) noexcept {
    try {
        saveFile(path, content, mode);
    } catch (const std::system_error& e) {
        ec = e.code();
        return false;
    } catch (const std::bad_alloc&) {
        ec = std::make_error_code(std::errc::not_enough_memory);
        return false;
    } catch (const std::exception&) {
        // This isn't exactly right, but good enough for now
        ec = std::make_error_code(std::errc::io_error);
        return false;
    }
    return true;
}

void saveFileAtomic(const std::filesystem::path& path,
                    std::string_view content) {
    auto tempfile = path;
    tempfile += ".tmp";
    try {
        FileSink sink(tempfile);
        sink.sink(content);
        // close() fsyncs the file before closing it
        sink.close();
        std::filesystem::rename(tempfile, path);
    } catch (const std::exception&) {
        std::error_code ec;
        std::filesystem::remove(tempfile, ec);
        throw;
    }
    // And finally sync the parent directory.
    fsyncDirectory(path.parent_path());
}

bool saveFileAtomic(const std::filesystem::path& path,
                    std::string_view content,
                    std::error_code& ec) noexcept {
    try {
        saveFileAtomic(path, content);
    } catch (const std::system_error& e) {
        ec = e.code();
        return false;
    } catch (const std::bad_alloc&) {
        ec = std::make_error_code(std::errc::not_enough_memory);
        return false;
    } catch (const std::exception&) {
        ec = std::make_error_code(std::errc::io_error);
        return false;
    }
    return true;
}
} // namespace cb::io
/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <fmt/format.h>
#include <gsl/gsl-lite.hpp>
#include <platform/file_sink.h>

namespace cb::io {
FileSink::FileSink(std::filesystem::path path,
                   Mode mode,
                   std::size_t fsync_interval)
    : filename(std::move(path)), fsync_interval(fsync_interval) {
    if (mode == Mode::Append) {
        fp = fopen(filename.string().c_str(), "ab");
    } else {
        fp = fopen(filename.string().c_str(), "wb");
    }

    if (fp == nullptr) {
        throw std::system_error(
                errno,
                std::system_category(),
                fmt::format("Failed to open file '{}'", filename.string()));
    }
    (void)std::setvbuf(fp, nullptr, _IONBF, 0);
}

void FileSink::sink(std::string_view data) {
    Expects(fp);
    if (data.empty()) {
        return;
    }
    std::size_t offset = 0;
    while (offset < data.size()) {
        auto chunk = std::min(data.size() - offset, fsync_interval);
        if (fwrite(data.data() + offset, chunk, 1, fp) != 1) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    fmt::format("Failed to write to file '{}' at offset {}",
                                filename.string(),
                                bytes_written));
        }

        bytes_written += chunk;
        bytes_written_since_flush += chunk;
        offset += chunk;

        if (bytes_written_since_flush >= fsync_interval) {
            fsync();
        }
    }
}

std::size_t FileSink::fsync() {
    Expects(fp);
    if (bytes_written_since_flush) {
        if (::fsync(fileno(fp)) == -1) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    fmt::format("Failed to fsync file '{}' at offset {}",
                                filename.string(),
                                bytes_written));
        }
        bytes_written_since_flush = 0;
    }
    return bytes_written;
}

std::size_t FileSink::close() {
    Expects(fp);
    if (fclose(fp) != 0) {
        throw std::system_error(
                errno,
                std::system_category(),
                fmt::format("Failed to close file '{}'", filename.string()));
    }
    fp = nullptr;
    return bytes_written;
}

FileSink::~FileSink() {
    if (fp) {
        try {
            close();
        } catch (const std::exception&) {
            // Can't throw from the destructor
        }
    }
}
} // namespace cb::io

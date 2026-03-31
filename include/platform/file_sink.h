/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <platform/io_hint.h>
#include <platform/sink.h>

#include <cstdio>
#include <filesystem>
#include <limits>
#include <string_view>

namespace cb::io {
/**
 * A sink which writes to a file and throws std::system_error on failures to
 * write the data to the file.
 */
class FileSink : public Sink {
public:
    enum class Mode {
        /// Append to the file if it exists, otherwise create a new file
        Append,
        /// Truncate the file if it exists, otherwise create a new file
        Truncate
    };

    FileSink() = delete;
    FileSink(const FileSink&) = delete;
    /**
     * Create a new instance of the FileSink
     *
     * @param path The name of the file to write
     * @param mode The mode to open the file in
     * @param fsync_interval The interval between fsync calls
     * @param io_hint The I/O access pattern hint to give the kernel for this
     *                file. If set to IoHint::DontNeed the kernel will be
     *                advised that the data will not be needed again and can be
     *                evicted from the buffer cache immediately after being
     *                written (and as part of flush an explicit call to drop
     *                pages is made)
     * @throws std::system_error if the file cannot be opened
     */
    explicit FileSink(std::filesystem::path path,
                      Mode mode = Mode::Truncate,
                      std::size_t fsync_interval =
                              std::numeric_limits<std::size_t>::max(),
                      IoHint io_hint = IoHint::Normal);

    /// Close the file (any errors is silently ignored)
    ~FileSink() override;

    /**
     * Write a blob of data to the end of the file
     * @param data The data to write to the file
     */
    void sink(std::string_view data) override;

    /**
     * (Explicit) Flush any uncommited data to the file system
     *
     * @return The (total) number of bytes written to the file since the
     *         creation of the object
     * @throws std::system_error if there is an error writing the data to the
     *                           file
     */
    std::size_t fsync() override;

    /**
     * Close the file (flushing any pending data). Trying to write to the file
     * after the file is closed will throw an exception.
     *
     * @return The number of bytes written to the file
     * @throws std::system_error if there is an error writing the data to the
     *                           file
     */
    std::size_t close() override;

    /// Get the number of bytes written to the file so far
    std::size_t getBytesWritten() const override {
        return bytes_written;
    }

protected:
    // @return errno if fsync fails, 0 if successful
    int fsync_no_throw() noexcept;

    const std::filesystem::path filename;
    const std::size_t fsync_interval;
    const IoHint io_hint;
    FILE* fp = nullptr;
    std::size_t bytes_written = 0;
    std::size_t bytes_written_since_flush = 0;
};
} // namespace cb::io

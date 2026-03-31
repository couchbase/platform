/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/**
 * @file io_hint.h
 * @brief Kernel I/O advise hints for optimized memory and buffer cache usage.
 *
 * This module provides a portable interface for giving the kernel hints about
 * how application code intends to access file data. On Linux platforms, these
 * hints are implemented using posix_fadvise(2), which allows the kernel to
 * optimize memory management and buffer cache behavior accordingly.
 *
 * Common use cases:
 * - Signal DontNeed after fsync to allow immediate page eviction
 * - Indicate Sequential access patterns for read-ahead optimization
 * - Mark data as Random to disable read-ahead caching
 * - Use WillNeed for important data that should remain in cache
 *
 * On non-Linux platforms, the functions are no-ops, ensuring portable code.
 */

#pragma once

#include <sys/types.h>
#include <string_view>
#include <system_error>

namespace cb::io {

/**
 * @enum IoHint
 * @brief I/O access patterns and memory usage hints for the kernel.
 *
 * These hints inform the kernel about how file data will be accessed,
 * enabling it to optimize memory allocation and page cache behavior.
 */
enum class IoHint {
    /// Normal access pattern with default kernel behavior
    Normal,

    /// Data will be accessed sequentially; enables read-ahead optimization
    Sequential,

    /// Data will be accessed randomly; disables read-ahead
    Random,

    /// Data will not be reused after being accessed (can be evicted sooner)
    NoReuse,

    /// Data will be needed again; should remain in buffer cache
    WillNeed,

    /// Data will not be needed again; kernel may evict pages immediately
    DontNeed,
};

std::string_view format_as(IoHint hint) noexcept;

/**
 * @brief Give kernel I/O advice for a specific file range with error handling.
 *
 * Advises the kernel about intended access patterns for a specific byte range
 * of a file. This variant captures errors in an std::error_code for callers
 * that prefer exception-free error handling.
 *
 * @param fd File descriptor of an open file
 * @param offset Byte offset within the file
 * @param size Number of bytes from offset (0 means to end of file)
 * @param hints IoHint flags describing access pattern
 * @param ec Output error_code; set to 0 on success, system error on failure
 * @return true on success, false on failure (check ec for details)
 *
 * @note On non-Linux platforms, always returns true and does not set ec.
 * @note The advice is a hint; the kernel may choose to ignore it.
 */
bool giveKernelIoAdvise(int fd,
                        off_t offset,
                        off_t size,
                        IoHint hints,
                        std::error_code& ec) noexcept;

/**
 * @brief Give kernel I/O advice for entire file with error handling.
 *
 * Advises the kernel about intended access patterns for the entire file.
 * Equivalent to giveKernelIoAdvise(fd, 0, 0, hints, ec).
 *
 * @param fd File descriptor of an open file
 * @param hints IoHint flags describing access pattern
 * @param ec Output error_code; set to 0 on success, system error on failure
 * @return true on success, false on failure (check ec for details)
 *
 * @note On non-Linux platforms, always returns true and does not set ec.
 */
bool giveKernelIoAdvise(int fd, IoHint hints, std::error_code& ec) noexcept;

/**
 * @brief Give kernel I/O advice for a specific file range (throws on error).
 *
 * Advises the kernel about intended access patterns for a specific byte range.
 * This variant throws std::system_error on failure.
 *
 * @param fd File descriptor of an open file
 * @param offset Byte offset within the file
 * @param size Number of bytes from offset (0 means to end of file)
 * @param hints IoHint flags describing access pattern
 *
 * @throws std::system_error if the kernel call fails
 * @note On non-Linux platforms, this is a no-op.
 */
void giveKernelIoAdvise(int fd, off_t offset, off_t size, IoHint hints);

/**
 * @brief Give kernel I/O advice for entire file (throws on error).
 *
 * Advises the kernel about intended access patterns for the entire file.
 * Equivalent to giveKernelIoAdvise(fd, 0, 0, hints).
 *
 * @param fd File descriptor of an open file
 * @param hints IoHint flags describing access pattern
 *
 * @throws std::system_error if the kernel call fails
 * @note On non-Linux platforms, this is a no-op.
 */
void giveKernelIoAdvise(int fd, IoHint hints);

} // namespace cb::io
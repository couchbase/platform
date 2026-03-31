/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/io_hint.h>

#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#endif

namespace cb::io {

std::string_view format_as(IoHint hint) noexcept {
    using namespace std::string_view_literals;
    switch (hint) {
    case IoHint::Normal:
        return "normal"sv;
    case IoHint::Sequential:
        return "sequential"sv;
    case IoHint::Random:
        return "random"sv;
    case IoHint::NoReuse:
        return "no-reuse"sv;
    case IoHint::WillNeed:
        return "will-need"sv;
    case IoHint::DontNeed:
        return "dont-need"sv;
    }
    return "Invalid IoHint"sv;
}

#if defined(__linux__) || defined(__APPLE__)
bool giveKernelIoAdvise(int fd,
                        off_t offset,
                        off_t size,
                        IoHint hint,
                        std::error_code& ec) noexcept {
#ifdef __linux__
    int flags = 0;
    switch (hint) {
    case IoHint::Normal:
        flags = POSIX_FADV_NORMAL;
        break;
    case IoHint::Sequential:
        flags = POSIX_FADV_SEQUENTIAL;
        break;
    case IoHint::Random:
        flags = POSIX_FADV_RANDOM;
        break;
    case IoHint::NoReuse:
        flags = POSIX_FADV_NOREUSE;
        break;
    case IoHint::WillNeed:
        flags = POSIX_FADV_WILLNEED;
        break;
    case IoHint::DontNeed:
        flags = POSIX_FADV_DONTNEED;
    }
    auto rv = posix_fadvise(fd, offset, size, flags);
    ec = std::make_error_code(static_cast<std::errc>(rv));
    return rv == 0;
#else
    int command = 0;
    int flags = 0;
    switch (hint) {
    case IoHint::Normal:
        [[fallthrough]];
    case IoHint::WillNeed:
        [[fallthrough]];
    case IoHint::DontNeed:
        return true;

    case IoHint::Sequential:
        command = F_RDAHEAD;
        flags = 1;
        break;
    case IoHint::Random:
        command = F_RDAHEAD;
        flags = 0;
        break;
    case IoHint::NoReuse:
        command = F_NOCACHE;
        flags = 1;
        break;
    }

    if (fcntl(fd, command, flags) == -1) {
        ec = std::make_error_code(static_cast<std::errc>(errno));
        return false;
    }

    return true;
#endif
}

bool giveKernelIoAdvise(int fd, IoHint hint, std::error_code& ec) noexcept {
    return giveKernelIoAdvise(fd, 0, 0, hint, ec);
}

void giveKernelIoAdvise(int fd, off_t offset, off_t size, IoHint hint) {
    std::error_code ec;
    if (!giveKernelIoAdvise(fd, offset, size, hint, ec)) {
        throw std::system_error(
                ec.value(), std::system_category(), "giveKernelIoAdvise");
    }
}

void giveKernelIoAdvise(int fd, IoHint hint) {
    giveKernelIoAdvise(fd, 0, 0, hint);
}
#else

bool giveKernelIoAdvise(int, off_t, off_t, IoHint, std::error_code&) noexcept {
    return true;
}

bool giveKernelIoAdvise(int, IoHint, std::error_code&) noexcept {
    return true;
}

void giveKernelIoAdvise(int, off_t, off_t, IoHint) {
}

void giveKernelIoAdvise(int, IoHint) {
}
#endif

} // namespace cb::io

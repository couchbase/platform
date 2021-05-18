/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/**
 * Helper functions to throw exceptions which record the backtrace of where
 * they were thrown from.
 * This backtrace can later be retrieved when the exception is caught.
 *
 * Usage:
 *
 *     try {
 *         ...
 *         if (bad_thing_happened) {
 *             cb::throwWithTrace(std::logic_error("bad thing!");
 *         }
 *         ...
 *     } catch (const std::logic_error& e) {
 *         if (auto* st = cb::getBacktrace(e)) {
 *             print_backtrace_frames(*st, [](const auto* frame) {
 *                 std::cerr << frame << "\n";
 *             });
 *         }
 *     }
 *
 */

#pragma once


#if defined(__APPLE__)
// https://github.com/boostorg/exception/issues/37 - on macOS and setting
// default symbol visibility to "hidden", get_error_info() fails across shared
// object boundaries. This is due to the RTTI implementation which requires
// typeinfo symbols to have vague linkage - i.e. they must be external in all
// binaries.
// As a workaround until above Boost issue is fixed, explicitly forward-declare
// are affected types with default visibility (PLATFORM_PUBLIC_API).
namespace boost {
template <class Tag, class T>
class error_info;
};
namespace boost {
namespace exception_detail {
class error_info_base;
};
}; // namespace boost
namespace boost {
namespace stacktrace {
class frame;
template <class Allocator>
class basic_stacktrace;
}; // namespace stacktrace
}; // namespace boost
#endif // defined(APPLE)

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>
// On WIN32, boost/stacktrace.hpp includes some Windows system headers,
// including combaseapi.h which defines a macro 'interface' for some COM
// nonsense, and there doesn't appear to be any way to avoid this macro
// being defined. This conflicts with various variables we have in the code
// named 'interface', so to avoid compile errors we un-define it here.
// Points for how long this took to figure out...
#if defined(WIN32)
#if defined(interface)
#undef interface
#endif
// Similar issue with 'uuid_t', defined in rpcdce.h
#if defined(uuid_t)
#undef uuid_t
#endif
#endif // WIN32

#include <folly/CPortability.h>

namespace cb {

struct tag_stacktrace;
using traced =
        boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>;

/**
 * Throws the specified exception, recording the backtrace of where the
 * exception was thrown from.
 *
 * The thrown exception will be a subclass of the passed in exception (so can
 * still be caught via 'catch (foo_exception&)`.
 *
 * The additional backtrace information can be obtained via:
 *     cb::getErrorInfo(e)
 *
 * @param e Exception to throw.
 *
 * Marked as NOINLINE to ensure we see explicitly where this function was called
 * in the recorded backtrace.
 */
template <class T>
FOLLY_NOINLINE void throwWithTrace(const T& exception) {
    throw boost::enable_error_info(exception)
            << traced(boost::stacktrace::stacktrace());
}

/**
 * Attempt to obtain the backtrace from whan an exception was thrown. If
 * the exception was thrown via `throwWithTrace` it will return a non-null
 * pointer which can be passed to an output stream.
 *
 * @param exception Exception to lookup backtrace from
 * @return Pointer to stacktrace if present, else nullptr.
 */
template <class T>
const boost::stacktrace::stacktrace* getBacktrace(T& exception) {
    return boost::get_error_info<traced>(exception);
}

} // namespace cb

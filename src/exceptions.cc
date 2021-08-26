/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/exceptions.h>

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

namespace cb {

struct tag_stacktrace;
using traced =
        boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>;

template <class T>
FOLLY_NOINLINE void throwWithTrace(const T& exception) {
    throw boost::enable_error_info(exception)
            << traced(boost::stacktrace::stacktrace());
}

// Add required explicit instantiations of throwWithTrace here -
// will need one per exception type thrown via throwWithTrace():
template void throwWithTrace(const std::underflow_error&);
template void throwWithTrace(const std::logic_error&);
template void throwWithTrace(const std::runtime_error&);

template <class T>
const boost::stacktrace::stacktrace* getBacktrace(T& exception) {
    return boost::get_error_info<traced>(exception);
}

// Add required explicit instantiations of getBacktrace here -
// will need one per exception type passed to getBacktrace():
using boost_st = boost::stacktrace::stacktrace;

template const boost_st* getBacktrace(const std::exception& exception);
template const boost_st* getBacktrace(const std::underflow_error& exception);

} // namespace cb

/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/cb_arena_malloc.h>
#include <platform/sysinfo.h>

#include <type_traits>

namespace cb {

ArenaMallocGuard::ArenaMallocGuard(const ArenaMallocClient& client) {
    ArenaMalloc::switchToClient(client);
}

ArenaMallocGuard::~ArenaMallocGuard() {
    ArenaMalloc::switchFromClient();
}

NoArenaGuard::NoArenaGuard() : previous(ArenaMalloc::switchFromClient()) {
}

NoArenaGuard::~NoArenaGuard() {
    ArenaMalloc::switchToClient(previous);
}

void ArenaMallocClient::setEstimateUpdateThreshold(size_t maxDataSize,
                                                   float percentage) {
    // Set the threshold to the smaller of the % calculation and max u32
    estimateUpdateThreshold = uint32_t(std::min(
            size_t(std::numeric_limits<uint32_t>::max()),
            size_t(maxDataSize * (percentage / 100.0) / cb::get_cpu_count())));
}

std::ostream& operator<<(std::ostream& os, const FragmentationStats& stats) {
    os << "allocated:" << stats.getAllocatedBytes()
       << ", resident:" << stats.getResidentBytes()
       << ", fragmentation:" << stats.getFragmentationRatio();
    return os;
}

#if defined(HAVE_JEMALLOC)
template class _ArenaMalloc<JEArenaMalloc>;
#else
template class _ArenaMalloc<SystemArenaMalloc>;
#endif

} // namespace cb

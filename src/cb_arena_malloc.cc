/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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

void ArenaMallocClient::setEstimateUpdateThreshold(size_t maxDataSize,
                                                   float percentage) {
    // Set the threshold to the smaller of the % calculation and max u32
    estimateUpdateThreshold = uint32_t(std::min(
            size_t(std::numeric_limits<uint32_t>::max()),
            size_t(maxDataSize * (percentage / 100.0) / cb::get_cpu_count())));
}

#if defined(HAVE_JEMALLOC)
template class PLATFORM_PUBLIC_API _ArenaMalloc<JEArenaMalloc>;
#else
template class PLATFORM_PUBLIC_API _ArenaMalloc<SystemArenaMalloc>;
#endif

} // namespace cb

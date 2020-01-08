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

#include "relaxed_atomic.h"
#include <platform/je_arena_malloc.h>

#include <folly/CachelinePadded.h>
#include <jemalloc/jemalloc.h>

#include <stdexcept>
#include <string>
#include <system_error>

// We are storing the arena in a uint16_t, assert that this constant is as
// expected, MALLCTL_ARENAS_ALL -1 is the largest possible arena ID
static_assert(MALLCTL_ARENAS_ALL == 4096,
              "je_malloc MALLCTL_ARENAS_ALL is not the expected 4096");

namespace cb {

/// Overrides any client tcache wishes
static bool tcacheEnabled{true};

/**
 * Structure storing data for the currently executing client
 */
struct CurrentClient {
    void setNoClient() {
        mallocFlags = 0;
        index = NoClientIndex;
    }

    void setup(int mallocFlags, uint8_t index) {
        this->mallocFlags = mallocFlags;
        this->index = index;
    }

    /**
     * The flags to be passed to all je_malloc 'x' calls, this is where the
     * current arena is stored and tcache id (if enabled).
     */
    int mallocFlags{0};

    /**
     * The index of the currently switched-to client, used for updating client
     * stat counters (e.g. the mem_used counters)
     */
    uint8_t index{NoClientIndex};
};

/**
 * ThreadLocalData
 * currentClient - stores the jemalloc flags and index of the currently
 *                 'executing' ArenaMallocClient. This is read by all allocation
 *                 methods for pushing the correct flags (arena) to jemalloc.
 * tcacheIds - An array of jemalloc thread cache identifiers, each thread:client
 *             needs its own id.
 */
class ThreadLocalData {
public:
    /**
     * Destruct the store which results in je_mallctl(tcache.destroy) for each
     * tcache in the store.
     */
    ~ThreadLocalData() noexcept(false);

    /**
     * Get the ThreadLocalData for the calling thread. This method will call
     * make() if no ThreadLocalData exists yet.
     */
    static ThreadLocalData& get();

    /**
     * @return a tcache to use for the given client
     */
    uint16_t getTCacheID(const ArenaMallocClient& client);

    CurrentClient& getCurrentClient() {
        return currentClient;
    }

private:
    /**
     * Make a new ThreadLocalData, this is always created in the default arena
     */
    static ThreadLocalData* make();
    CurrentClient currentClient;

    /// Actual array of identifiers, 0 means no tcache has been created
    std::array<uint16_t, ArenaMallocMaxClients> tcacheIds;
};

/// unique_ptr destructor for the thread_local tcache store
struct ThreadLocalDataDestroy {
    void operator()(ThreadLocalData* ptr);
};

static thread_local std::unique_ptr<ThreadLocalData, ThreadLocalDataDestroy>
        tld;

/**
 * A structure for mapping a client to an arena
 */
class Clients {
public:
    struct Client {
        void reset(int arena) {
            used = false;
            this->arena = arena;
        }
        int arena = 0;
        bool used = false;
    };

    using ClientArray =
            folly::Synchronized<std::array<Client, ArenaMallocMaxClients>>;
    static ClientArray& get();

private:
    static ClientArray* make();
    /// unique_ptr destructor for the thread_local tcache store
    struct ClientArrayDestroy {
        void operator()(ClientArray* ptr);
    };
    static std::unique_ptr<ClientArray, ClientArrayDestroy> clients;
};

/// @return a jemalloc arena
static int makeArena() {
    unsigned arena = 0;
    size_t sz = sizeof(unsigned);
    int rv = je_mallctl("arenas.create", (void*)&arena, &sz, nullptr, 0);

    if (rv != 0) {
        throw std::runtime_error(
                "JEArenaMalloc::makeArena not create arena. rv:" +
                std::to_string(rv));
    }
    return arena;
}

template <>
PLATFORM_PUBLIC_API ArenaMallocClient
JEArenaMalloc::registerClient(bool threadCache) {
    auto lockedClients = Clients::get().wlock();
    for (uint8_t index = 0; index < lockedClients->size(); index++) {
        auto& client = lockedClients->at(index);
        if (!client.used) {
            if (client.arena == 0) {
                client.arena = makeArena();
            }

            // We use arena 0 as no arena and don't expect it to be created
            if (client.arena == 0) {
                throw std::runtime_error(
                        "JEArenaMalloc::registerClient did not expect to have "
                        "arena 0");
            }
            client.used = true;
            ArenaMallocClient newClient{
                    client.arena, index, threadCache && tcacheEnabled};
            clientRegistered(newClient);
            return newClient;
        }
    }
    throw std::runtime_error(
            "JEArenaMalloc::registerClient no available indices");
}

template <>
PLATFORM_PUBLIC_API void JEArenaMalloc::unregisterClient(
        const ArenaMallocClient& client) {
    auto lockedClients = Clients::get().wlock();
    auto& c = lockedClients->at(client.index);
    if (!c.used) {
        throw std::invalid_argument(
                "JEArenaMalloc::unregisterClient client is not in-use "
                "client.index:" +
                std::to_string(client.index));
    }
    // Reset the state, but we re-use the arena
    c.reset(c.arena);
}

template <>
PLATFORM_PUBLIC_API void JEArenaMalloc::switchToClient(
        const ArenaMallocClient& client, bool tcache) {
    auto& tld = ThreadLocalData::get();
    if (client.index == NoClientIndex) {
        tld.getCurrentClient().setup(
                client.threadCache && tcacheEnabled ? 0 : MALLOCX_TCACHE_NONE,
                NoClientIndex);
        return;
    }

    int tcacheFlags = MALLOCX_TCACHE_NONE;
    // client can change tcache setting via their client object or for a single
    // swicthToClient call.
    // AND all inputs together, if any is false then no tcache
    if (tcache && client.threadCache && tcacheEnabled) {
        tcacheFlags = MALLOCX_TCACHE(tld.getTCacheID(client));
    } else {
        // tcache is disabled but we still need to trigger a call to initialise
        // the per thread tracker, which is a side affect of tcache creation.
        // So call get (which will create a tcache), but we don't add it to the
        // flags so tcache is still MALLOCX_TCACHE_NONE
        tld.getTCacheID(client);
    }

    // Set the malloc flags to the correct arena + tcache setting and set the
    // client index
    tld.getCurrentClient().setup(MALLOCX_ARENA(client.arena) | tcacheFlags,
                                 client.index);
}

template <>
PLATFORM_PUBLIC_API void JEArenaMalloc::switchFromClient() {
    // Set to 0, no client, all tracking/allocations go to default arena/tcache
    switchToClient({0, NoClientIndex, tcacheEnabled}, tcacheEnabled);
}

template <>
void* JEArenaMalloc::malloc(size_t size) {
    if (size == 0) {
        size = 8;
    }
    auto& c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, size);
    return je_mallocx(size, c.mallocFlags);
}

template <>
void* JEArenaMalloc::calloc(size_t nmemb, size_t size) {
    auto& c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, size);
    return je_mallocx(nmemb * size, c.mallocFlags | MALLOCX_ZERO);
}

template <>
void* JEArenaMalloc::realloc(void* ptr, size_t size) {
    if (size == 0) {
        size = 8;
    }

    auto& c = ThreadLocalData::get().getCurrentClient();

    if (!ptr) {
        memAllocated(c.index, size);
        return je_mallocx(size, c.mallocFlags);
    }

    memDeallocated(c.index, ptr);
    memAllocated(c.index, size);
    return je_rallocx(ptr, size, c.mallocFlags);
}

template <>
void JEArenaMalloc::free(void* ptr) {
    if (ptr) {
        auto& c = ThreadLocalData::get().getCurrentClient();
        memDeallocated(c.index, ptr);
        je_dallocx(ptr, c.mallocFlags);
    }
}

template <>
void JEArenaMalloc::sized_free(void* ptr, size_t size) {
    if (ptr) {
        auto& c = ThreadLocalData::get().getCurrentClient();
        memDeallocated(c.index, size);
        je_sdallocx(ptr, size, c.mallocFlags);
    }
}

template <>
size_t JEArenaMalloc::malloc_usable_size(const void* ptr) {
    return je_sallocx(const_cast<void*>(ptr), 0);
}

template <>
bool JEArenaMalloc::setTCacheEnabled(bool value) {
    bool oldValue = tcacheEnabled;
    tcacheEnabled = value;
    return oldValue;
}

template <>
bool JEArenaMalloc::getProperty(const char* name, size_t& value) {
    size_t size = sizeof(size_t);
    return je_mallctl(name, &value, &size, NULL, 0);
}

template <>
int JEArenaMalloc::setProperty(const char* name,
                               const void* newp,
                               size_t newlen) {
    return je_mallctl(name, nullptr, 0, const_cast<void*>(newp), newlen);
}

void ThreadLocalDataDestroy::operator()(ThreadLocalData* ptr) {
    ptr->~ThreadLocalData();
    // de-allocate from the default arena
    je_dallocx((void*)ptr, 0);
}

ThreadLocalData::~ThreadLocalData() noexcept(false) {
    for (auto tc : tcacheIds) {
        if (tc) {
            unsigned tcache = tc;
            size_t sz = sizeof(unsigned);
            if (je_mallctl("tcache.destroy", nullptr, 0, (void*)&tcache, sz) !=
                0) {
                throw std::logic_error(
                        "JEArenaMalloc::TCacheDestroy: Could not "
                        "destroy "
                        "tcache");
            }
        }
    }
}

ThreadLocalData* ThreadLocalData::make() {
    // Always create in the default arena/cache (and zero init)
    auto* vptr =
            (ThreadLocalData*)je_mallocx(sizeof(ThreadLocalData), MALLOCX_ZERO);
    return new (vptr) ThreadLocalData();
}

ThreadLocalData& ThreadLocalData::get() {
    auto* arrayPtr = tld.get();
    if (!arrayPtr) {
        arrayPtr = make();
        tld.reset(arrayPtr);
    }
    return *arrayPtr;
}

uint16_t ThreadLocalData::getTCacheID(const ArenaMallocClient& client) {
    // If no tcache exists one must be created (id:0 means no tcache)
    if (!tcacheIds[client.index]) {
        unsigned tcache = 0;
        size_t sz = sizeof(unsigned);
        int rv = je_mallctl("tcache.create", (void*)&tcache, &sz, nullptr, 0);
        if (rv != 0) {
            throw std::runtime_error(
                    "JThreadLocalData::get: Could not create tcache. rv:" +
                    std::to_string(rv));
        }
        tcacheIds[client.index] = tcache;

        // We are using the creation of the tcache for this client as a trigger
        // that this is the first allocation for this client on this thread. In
        // this case we must call initialiseForNewThread which the tracker can
        // hook into and if required do any thread initialisation. Note that
        // this function will also ensure that the current client is NoClient
        JEArenaMalloc::initialiseForNewThread(client);
    }
    return tcacheIds[client.index];
}

Clients::ClientArray& Clients::get() {
    auto* arrayPtr = clients.get();
    if (!arrayPtr) {
        arrayPtr = make();
        clients.reset(arrayPtr);
    }
    return *arrayPtr;
}

Clients::ClientArray* Clients::make() {
    // Always create in the default arena/cache (and zero init)
    auto* vptr = (Clients::ClientArray*)je_mallocx(sizeof(Clients::ClientArray),
                                                   MALLOCX_ZERO);
    return new (vptr) Clients::ClientArray();
}

void Clients::ClientArrayDestroy::operator()(Clients::ClientArray* ptr) {
    ptr->~ClientArray();
    // de-allocate from the default arena
    je_dallocx((void*)ptr, 0);
}

std::unique_ptr<Clients::ClientArray, Clients::ClientArrayDestroy>
        Clients::clients;

} // namespace cb

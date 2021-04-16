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

#include <folly/lang/Aligned.h>
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
 *
 * Note this is a struct to reduce the number of tls_get_addr calls when it is
 * used. If this struct is made a class, or wrapped in a CachelinePadded, or
 * uses a class member all accesses to the object require multiple TLS reads
 * (a guard variable comes into play).
 * Using a much simpler struct/POD and the compiler does single calls
 * to retrieve the address of the per thread object.
 */
struct ThreadLocalData {
public:
    /**
     * @return a tcache to use for the given client
     */
    uint16_t getTCacheID(const ArenaMallocClient& client);

    CurrentClient& getCurrentClient() {
        return currentClient;
    }

    static ThreadLocalData& get() {
        static thread_local ThreadLocalData tld = {};
        return tld;
    }

private:
    CurrentClient currentClient = {0, NoClientIndex};

    /// Actual array of identifiers, value of 0 means no tcache has been created
    uint16_t tcacheIds[ArenaMallocMaxClients] = {0};
};

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
    /// Creates a ClientArray in the jemalloc 'default' arena
    static ClientArray* make();
    /// Destroys the ClientArray using jemalloc
    struct ClientArrayDestroy {
        void operator()(ClientArray* ptr);
    };
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
ArenaMallocClient JEArenaMalloc::registerClient(bool threadCache) {
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
            "JEArenaMalloc::registerClient all available arenas are allocated "
            "to clients");
}

template <>
void JEArenaMalloc::unregisterClient(const ArenaMallocClient& client) {
    auto lockedClients = Clients::get().wlock();
    auto& c = lockedClients->at(client.index);
    if (!c.used) {
        throw std::invalid_argument(
                "JEArenaMalloc::unregisterClient the client is not in-use "
                "client.index:" +
                std::to_string(client.index));
    }
    // Reset the state, but we re-use the arena
    c.reset(c.arena);
}

template <>
void JEArenaMalloc::switchToClient(const ArenaMallocClient& client,
                                   bool tcache) {
    if (client.index == NoClientIndex) {
        ThreadLocalData::get().getCurrentClient().setup(
                client.threadCache && tcacheEnabled ? 0 : MALLOCX_TCACHE_NONE,
                NoClientIndex);
        return;
    }

    int tcacheFlags = MALLOCX_TCACHE_NONE;
    // client can change tcache setting via their client object or for a single
    // swicthToClient call.
    // AND all inputs together, if any is false then no tcache
    if (tcache && client.threadCache && tcacheEnabled) {
        tcacheFlags =
                MALLOCX_TCACHE(ThreadLocalData::get().getTCacheID(client));
    } else {
        // tcache is disabled but we still need to trigger a call to initialise
        // the per thread tracker, which is a side affect of tcache creation.
        // So call get (which will create a tcache), but we don't add it to the
        // flags so tcache is still MALLOCX_TCACHE_NONE
        ThreadLocalData::get().getTCacheID(client);
    }

    // Set the malloc flags to the correct arena + tcache setting and set the
    // client index
    ThreadLocalData::get().getCurrentClient().setup(
            MALLOCX_ARENA(client.arena) | tcacheFlags, client.index);
}

template <>
void JEArenaMalloc::switchFromClient() {
    // Set to 0, no client, all tracking/allocations go to default arena/tcache
    switchToClient({0, NoClientIndex, tcacheEnabled}, tcacheEnabled);
}

template <>
void* JEArenaMalloc::malloc(size_t size) {
    if (size == 0) {
        size = 8;
    }
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, size);
    return je_mallocx(size, c.mallocFlags);
}

template <>
void* JEArenaMalloc::calloc(size_t nmemb, size_t size) {
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, nmemb * size);
    return je_mallocx(nmemb * size, c.mallocFlags | MALLOCX_ZERO);
}

template <>
void* JEArenaMalloc::realloc(void* ptr, size_t size) {
    if (size == 0) {
        size = 8;
    }

    auto c = ThreadLocalData::get().getCurrentClient();

    if (!ptr) {
        memAllocated(c.index, size);
        return je_mallocx(size, c.mallocFlags);
    }

    memDeallocated(c.index, ptr);
    memAllocated(c.index, size);
    return je_rallocx(ptr, size, c.mallocFlags);
}

template <>
void* JEArenaMalloc::aligned_alloc(size_t alignment, size_t size) {
    if (size == 0) {
        size = 8;
    }
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, size, std::align_val_t{alignment});
    return je_mallocx(size, c.mallocFlags | MALLOCX_ALIGN(alignment));
}

template <>
void JEArenaMalloc::free(void* ptr) {
    if (ptr) {
        auto c = ThreadLocalData::get().getCurrentClient();
        memDeallocated(c.index, ptr);
        je_dallocx(ptr, c.mallocFlags);
    }
}

template <>
void JEArenaMalloc::aligned_free(void* ptr) {
    // Normal jemalloc free method can be used.
    JEArenaMalloc::free(ptr);
}

template <>
void JEArenaMalloc::sized_free(void* ptr, size_t size) {
    if (ptr) {
        auto c = ThreadLocalData::get().getCurrentClient();
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
    return je_mallctl(name, &value, &size, nullptr, 0);
}

template <>
int JEArenaMalloc::setProperty(const char* name,
                               const void* newp,
                               size_t newlen) {
    return je_mallctl(name, nullptr, nullptr, const_cast<void*>(newp), newlen);
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
template <>
void JEArenaMalloc::releaseMemory() {
    setProperty("arena." STRINGIFY(MALLCTL_ARENAS_ALL) ".purge", nullptr, 0);
}

template <>
void JEArenaMalloc::releaseMemory(const ArenaMallocClient& client) {
    std::string purgeKey = "arena." + std::to_string(client.arena) + ".purge";
    setProperty(purgeKey.c_str(), nullptr, 0);
}

uint16_t ThreadLocalData::getTCacheID(const ArenaMallocClient& client) {
    // If no tcache exists one must be created (id:0 means no tcache)
    if (!tcacheIds[client.index]) {
        unsigned tcache = 0;
        size_t sz = sizeof(unsigned);
        int rv = je_mallctl("tcache.create", (void*)&tcache, &sz, nullptr, 0);
        if (rv != 0) {
            throw std::runtime_error(
                    "ThreadLocalData::getTCacheID: tcache.create failed rv:" +
                    std::to_string(rv));
        }
        tcacheIds[client.index] = tcache;

        // We need to be sure that all allocated tcaches are destroyed at thread
        // exit, do this by using a destruct function attached to a unique_ptr.
        struct ThreadLocalDataDestroy {
            void operator()(ThreadLocalData* ptr) {
                for (auto tc : ptr->tcacheIds) {
                    if (tc) {
                        unsigned tcache = tc;
                        size_t sz = sizeof(unsigned);
                        if (je_mallctl("tcache.destroy",
                                       nullptr,
                                       nullptr,
                                       (void*)&tcache,
                                       sz) != 0) {
                            throw std::logic_error(
                                    "JEArenaMalloc::ThreadLocalDataDestroy: "
                                    "Could not "
                                    "destroy "
                                    "tcache");
                        }
                    }
                }
            }
        };
        thread_local std::unique_ptr<ThreadLocalData, ThreadLocalDataDestroy>
                destroyTcache{this};
    }
    return tcacheIds[client.index];
}

Clients::ClientArray& Clients::get() {
    static std::unique_ptr<ClientArray, ClientArrayDestroy> clients{make()};
    return *clients.get();
}

Clients::ClientArray* Clients::make() {
    // Always create in the default arena/cache (and zero init)
    auto* vptr = (Clients::ClientArray*)je_mallocx(sizeof(Clients::ClientArray),
                                                   MALLOCX_ZERO);
    if (vptr == nullptr) {
        throw std::runtime_error("Clients::make je_mallocx returned nullptr");
    }
    return new (vptr) Clients::ClientArray();
}

void Clients::ClientArrayDestroy::operator()(Clients::ClientArray* ptr) {
    ptr->~ClientArray();
    // de-allocate from the default arena
    je_dallocx((void*)ptr, 0);
}

} // namespace cb

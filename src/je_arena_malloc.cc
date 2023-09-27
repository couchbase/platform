/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "relaxed_atomic.h"
#include <platform/je_arena_malloc.h>

#include <folly/lang/Aligned.h>
#include <jemalloc/jemalloc.h>

#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>

// We are storing the arena in a uint16_t, assert that this constant is as
// expected, MALLCTL_ARENAS_ALL -1 is the largest possible arena ID
static_assert(MALLCTL_ARENAS_ALL <= std::numeric_limits<uint16_t>::max(),
              "je_malloc MALLCTL_ARENAS_ALL is not in the expected range");

namespace cb {

/// Overrides any client tcache wishes
static bool tcacheEnabled{true};

JEArenaMallocBase::CurrentClient::CurrentClient(int mallocFlags,
                                                uint8_t index,
                                                MemoryDomain domain)
    : mallocFlags(mallocFlags), index(index), domain(domain) {
}

void JEArenaMallocBase::CurrentClient::setNoClient() {
    mallocFlags = 0;
    index = NoClientIndex;
}

void JEArenaMallocBase::CurrentClient::setup(int mallocFlags,
                                             uint8_t index,
                                             cb::MemoryDomain domain) {
    this->mallocFlags = mallocFlags;
    this->index = index;
    this->domain = domain;
}

MemoryDomain JEArenaMallocBase::CurrentClient::setDomain(MemoryDomain domain) {
    auto currentDomain = this->domain;
    this->domain = domain;
    return currentDomain;
}

// Note: we can exceed uint64_t - just incurs an extra TLS read on what is
// quite hot code.
static_assert(sizeof(JEArenaMallocBase::CurrentClient) == sizeof(uint64_t),
              "Expected CurrentClient to be sizeof(uint64_t)");

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

    JEArenaMallocBase::CurrentClient& getCurrentClient() {
        return currentClient;
    }

    static ThreadLocalData& get() {
        static thread_local ThreadLocalData tld = {};
        return tld;
    }

private:
    JEArenaMallocBase::CurrentClient currentClient;

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

JEArenaMalloc::ClientHandle switchToClientImpl(uint8_t index,
                                               MemoryDomain domain,
                                               int mallocFlags) {
    auto& currentClient = ThreadLocalData::get().getCurrentClient();
    auto previous = currentClient;

    currentClient.setup(mallocFlags, index, domain);
    return previous;
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
JEArenaMalloc::ClientHandle JEArenaMalloc::switchToClient(
        const ArenaMallocClient& client, MemoryDomain domain, bool tcache) {
    if (client.index == NoClientIndex) {
        return switchToClientImpl(
                NoClientIndex,
                cb::MemoryDomain::None,
                client.threadCache && tcacheEnabled ? 0 : MALLOCX_TCACHE_NONE);
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
    return switchToClientImpl(
            client.index, domain, MALLOCX_ARENA(client.arena) | tcacheFlags);
}

template <>
JEArenaMalloc::ClientHandle JEArenaMalloc::switchToClient(
        const ClientHandle& client) {
    return switchToClientImpl(client.index, client.domain, client.mallocFlags);
}

template <>
MemoryDomain JEArenaMalloc::setDomain(MemoryDomain domain) {
    return ThreadLocalData::get().getCurrentClient().setDomain(domain);
}

template <>
JEArenaMalloc::ClientHandle JEArenaMalloc::switchFromClient() {
    // Set arena to 0 - which JEMALLOC means use auto select arena
    // Index to the special NoClientIndex (so no tracking occurs)
    // And domain to none (no use when tracking is off)
    // Now all allocations go to default arena and we don't do any counting
    return switchToClient(
            ArenaMallocClient{0 /*arena*/, NoClientIndex, tcacheEnabled},
            cb::MemoryDomain::None,
            tcacheEnabled);
}

template <>
void* JEArenaMalloc::malloc(size_t size) {
    if (size == 0) {
        size = 8;
    }
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, c.domain, size);
    return je_mallocx(size, c.mallocFlags);
}

template <>
void* JEArenaMalloc::calloc(size_t nmemb, size_t size) {
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, c.domain, nmemb * size);
    return je_mallocx(nmemb * size, c.mallocFlags | MALLOCX_ZERO);
}

template <>
void* JEArenaMalloc::realloc(void* ptr, size_t size) {
    if (size == 0) {
        size = 8;
    }

    auto c = ThreadLocalData::get().getCurrentClient();

    if (!ptr) {
        memAllocated(c.index, c.domain, size);
        return je_mallocx(size, c.mallocFlags);
    }

    memDeallocated(c.index, c.domain, ptr);
    memAllocated(c.index, c.domain, size);
    return je_rallocx(ptr, size, c.mallocFlags);
}

template <>
void* JEArenaMalloc::aligned_alloc(size_t alignment, size_t size) {
    if (size == 0) {
        size = 8;
    }
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, c.domain, size, std::align_val_t{alignment});
    return je_mallocx(size, c.mallocFlags | MALLOCX_ALIGN(alignment));
}

template <>
void JEArenaMalloc::free(void* ptr) {
    if (ptr) {
        auto c = ThreadLocalData::get().getCurrentClient();
        memDeallocated(c.index, c.domain, ptr);
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
        memDeallocated(c.index, c.domain, size);
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
bool JEArenaMalloc::getProperty(const char* name, unsigned& value) {
    size_t size = sizeof(unsigned);
    return je_mallctl(name, &value, &size, nullptr, 0);
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

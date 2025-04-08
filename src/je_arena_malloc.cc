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

#include <platform/backtrace.h>
#include <platform/je_arena_malloc.h>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <folly/Synchronized.h>
#include <jemalloc/jemalloc.h>
#include <platform/terminal_color.h>

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

/// Are additional checks on arena allocations enabled?
static const bool arenaDebugChecksEnabled{
        getenv("CB_ARENA_MALLOC_VERIFY_DEALLOC_CLIENT") != nullptr};

/**
 * Should Tcache be used?
 * @returns true if tcacheRequested is true, and tcache is not otherwise
 * inhibited.
 *
 * TCache is inhibited if either it's been globally disabled (via
 * setTCacheEnabled), or if arena debug checks are enabled. This is because
 * arena verification doesn't work with tcaches - while memory is initially
 * taken from the specified arena, if tcaches are enabled and the allocation is
 * freed, it may be returned to the tcache and *not* the arena. If the same
 * thread subsequently requests an allocation of the same size (from a
 * different arena), that thread can get memory from the tcache - i.e when
 * tcaches are enabled, a caller doesn't necessary get memory from the
 * specified arena. As a result when we attempt to verify that the
 * deallocating client is the same as the one which originally allocated it,
 * we can get a false error.
 */
static bool isTcacheEnabled(bool requested) {
    return tcacheEnabled && requested && !arenaDebugChecksEnabled;
}

JEArenaMallocBase::CurrentClient::CurrentClient(uint8_t index,
                                                MemoryDomain domain,
                                                uint16_t arena,
                                                int tcacheFlags)
    : tcacheFlags(tcacheFlags), arena(arena), index(index), domain(domain) {
}

MemoryDomain JEArenaMallocBase::CurrentClient::setDomain(MemoryDomain domain,
                                                         uint16_t arena) {
    auto currentDomain = this->domain;
    this->arena = arena;
    this->domain = domain;
    return currentDomain;
}

/**
 * Allocate a new arena which will have the allocator hooks replaced with our
 * own alloc/dalloc hooks (which just call the original hooks).
 *
 * @return the ID of the new arena
 */
static int makeArena();

int JEArenaMallocBase::CurrentClient::getMallocFlags() const {
    return MALLOCX_ARENA(arena) | tcacheFlags;
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

    ~ThreadLocalData() {
        // Clean up the thread local state in jemalloc.
        // This is a no-op if the thread state is not initialized or if
        // it is already cleaned up.
        // The call ensures that a resurrected state is not leaked. A
        // resurrected state occurs when the jemalloc TLS callback is called,
        // but then the thread does additional allocations/frees. This happens
        // deterministically on Windows, where the thread callback is always
        // called before the C++ thread_local destructors run.
        je_thread_cleanup();
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
        void reset() {
            used = false;
        }
        DomainToArena arenas;
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

/// How should arenas be assigned to domains?
enum class ArenaMode { SingleArena, OnePerDomain };

/**
 * Assign arena(s) to the specified domain to arena map, if not already
 * assigned (if arenas already assigned then keep previous assignment).
 * @param arenas DomainToArena mapping to populate.
 * @param arenaMode By what mode should arens be assigned.
 */
void assignClientArenas(DomainToArena& arenas, ArenaMode arenaMode) {
    switch (arenaMode) {
    case ArenaMode::SingleArena:
        // Use a single arena for all domains.
        if (arenas.front() == 0) {
            // No arena yet assigned, create one.
            int arena = makeArena();
            // We use arena 0 as no arena and don't expect it to be created
            if (arena == 0) {
                throw std::runtime_error(
                        "JEArenaMalloc::registerClient did not expect to have "
                        "arena 0");
            }
            arenas.fill(arena);
        }
        return;
    case ArenaMode::OnePerDomain:
        // Assign one arena per domain, uses more resource (Nx arenas per
        // client, but permits additional sanity-checks.
        for (auto& arena : arenas) {
            // Debug configuration, one arena per domain
            if (arena == 0) {
                arena = makeArena();
            }
            // We use arena 0 as no arena and don't expect it to be created
            if (arena == 0) {
                throw std::runtime_error(
                        "JEArenaMalloc::registerClient did not expect to have "
                        "arena 0");
            }
        }
        return;
    }
}

// Lookup arena of memory being freed, check it matches current
// client / domain.
static void verifyMemDeallocatedByCorrectClient(
        const JEArenaMallocBase::CurrentClient& client,
        void* ptr,
        size_t size) {
    unsigned allocatedFromArena = 0;
    size_t sz = sizeof(allocatedFromArena);
    int rv = je_mallctl(
            "arenas.lookup", &allocatedFromArena, &sz, &ptr, sizeof(void*));

    if (rv != 0) {
        throw std::runtime_error("JEArenaMalloc::cannot get arena:" +
                                 std::to_string(rv));
    }

    unsigned currentClientArena = client.arena;
    if (client.index != NoClientIndex &&
        allocatedFromArena != currentClientArena) {
        fmt::print(stderr,
                   "{}===ERROR===: JeArenaMalloc deallocation mismatch{}\n"
                   "\tMemory freed by client:{} domain:{} which is assigned "
                   "arena:{}, but memory was previously allocated from "
                   "arena:{} ({}).\n"
                   "\tAllocation address:{} size:{}\n",
                   cb::terminal::TerminalColor::Red,
                   cb::terminal::TerminalColor::Reset,
                   client.index,
                   client.domain,
                   currentClientArena,
                   allocatedFromArena,
                   allocatedFromArena == 0 ? "global arena"
                                           : "client-specific arena",
                   ptr,
                   size);
        print_backtrace_to_file(stderr);
        abort();
    }
}

JEArenaMalloc::ClientHandle switchToClientImpl(uint8_t index,
                                               MemoryDomain domain,
                                               uint16_t arena,
                                               int tcacheFlags) {
    auto& currentClient = ThreadLocalData::get().getCurrentClient();
    auto previous = currentClient;

    currentClient = {index, domain, arena, tcacheFlags};
    return previous;
}

template <>
ArenaMallocClient JEArenaMalloc::registerClient(bool threadCache) {
    auto lockedClients = Clients::get().wlock();
    for (uint8_t index = 0; index < lockedClients->size(); index++) {
        auto& client = lockedClients->at(index);
        if (!client.used) {
            assignClientArenas(client.arenas,
                               arenaDebugChecksEnabled
                                       ? ArenaMode::OnePerDomain
                                       : ArenaMode::SingleArena);
            client.used = true;

            ArenaMallocClient newClient{
                    client.arenas, index, isTcacheEnabled(threadCache)};
            clientRegistered(newClient, arenaDebugChecksEnabled);
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
    // Reset the state, but we re-use the arenas for the next time this is
    // used.
    c.reset();
}

template <>
uint8_t JEArenaMalloc::getCurrentClientIndex() {
    return ThreadLocalData::get().getCurrentClient().index;
}

template <>
uint16_t JEArenaMalloc::getCurrentClientArena() {
    return ThreadLocalData::get().getCurrentClient().arena;
}

template <>
JEArenaMalloc::ClientHandle JEArenaMalloc::switchToClient(
        const ArenaMallocClient& client, MemoryDomain domain, bool tcache) {
    if (client.index == NoClientIndex) {
        return switchToClientImpl(
                NoClientIndex,
                cb::MemoryDomain::None,
                /* arena */ 0,
                isTcacheEnabled(client.threadCache) ? 0 : MALLOCX_TCACHE_NONE);
    }

    int tcacheFlags = MALLOCX_TCACHE_NONE;
    // client can change tcache setting via their client object or for a single
    // swicthToClient call.
    // AND all inputs together, if any is false then no tcache
    if (isTcacheEnabled(tcache && client.threadCache)) {
        tcacheFlags =
                MALLOCX_TCACHE(ThreadLocalData::get().getTCacheID(client));
    } else {
        // tcache is disabled but we still need to trigger a call to initialise
        // the per thread tracker, which is a side affect of tcache creation.
        // So call get (which will create a tcache), but we don't add it to the
        // flags so tcache is still MALLOCX_TCACHE_NONE
        ThreadLocalData::get().getTCacheID(client);
    }
    return switchToClientImpl(client.index,
                              domain,
                              client.arenas.at(size_t(domain)),
                              tcacheFlags);
}

template <>
JEArenaMalloc::ClientHandle JEArenaMalloc::switchToClient(
        const ClientHandle& client) {
    return switchToClientImpl(
            client.index, client.domain, client.arena, client.tcacheFlags);
}

template <>
MemoryDomain JEArenaMalloc::setDomain(MemoryDomain domain) {
    auto& currentClient = ThreadLocalData::get().getCurrentClient();
    if (currentClient.index == NoClientIndex) {
        return ThreadLocalData::get().getCurrentClient().setDomain(domain, 0);
    }
    auto locked = Clients::get().rlock();
    const auto arenaForDomain =
            locked->at(currentClient.index).arenas.at(size_t(domain));
    return ThreadLocalData::get().getCurrentClient().setDomain(domain,
                                                               arenaForDomain);
}

template <>
JEArenaMalloc::ClientHandle JEArenaMalloc::switchFromClient() {
    // Set arena to 0 - which JEMALLOC means use auto select arena
    // Index to the special NoClientIndex (so no tracking occurs)
    // And domain to none (no use when tracking is off)
    // Now all allocations go to default arena and we don't do any counting
    return switchToClient(
            ArenaMallocClient{
                    DomainToArena{}, NoClientIndex, isTcacheEnabled(true)},
            cb::MemoryDomain::None,
            isTcacheEnabled(true));
}

template <>
void* JEArenaMalloc::malloc(size_t size) {
    if (size == 0) {
        size = 8;
    }
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, c.domain, size);
    return je_mallocx(size, c.getMallocFlags());
}

template <>
void* JEArenaMalloc::calloc(size_t nmemb, size_t size) {
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, c.domain, nmemb * size);
    return je_mallocx(nmemb * size, c.getMallocFlags() | MALLOCX_ZERO);
}

template <>
void* JEArenaMalloc::realloc(void* ptr, size_t size) {
    if (size == 0) {
        size = 8;
    }

    auto c = ThreadLocalData::get().getCurrentClient();

    if (!ptr) {
        memAllocated(c.index, c.domain, size);
        return je_mallocx(size, c.getMallocFlags());
    }

    memDeallocated(c.index, c.domain, ptr);
    memAllocated(c.index, c.domain, size);
    return je_rallocx(ptr, size, c.getMallocFlags());
}

template <>
void* JEArenaMalloc::aligned_alloc(size_t alignment, size_t size) {
    if (size == 0) {
        size = 8;
    }
    auto c = ThreadLocalData::get().getCurrentClient();
    memAllocated(c.index, c.domain, size, std::align_val_t{alignment});
    return je_mallocx(size, c.getMallocFlags() | MALLOCX_ALIGN(alignment));
}

template <>
void JEArenaMalloc::free(void* ptr) {
    if (ptr) {
        auto c = ThreadLocalData::get().getCurrentClient();
        if (arenaDebugChecksEnabled) {
            verifyMemDeallocatedByCorrectClient(c, ptr, je_sallocx(ptr, 0));
        }
        memDeallocated(c.index, c.domain, ptr);
        je_dallocx(ptr, c.getMallocFlags());
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
        if (arenaDebugChecksEnabled) {
            verifyMemDeallocatedByCorrectClient(c, ptr, size);
        }
        memDeallocated(c.index, c.domain, size);
        je_sdallocx(ptr, size, c.getMallocFlags());
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
    // TODO: Purge all areans (for each domain)? For now just purge primary.
    std::string purgeKey =
            "arena." +
            std::to_string(client.arenas.at(size_t(MemoryDomain::Primary))) +
            ".purge";
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
                // MB-58949: Ideally a thread should not be associated
                // with any client when it is destroyed, but there's
                // some cases where we don't have full control of the
                // given thread (e.g. RocksDB background threads) and
                // the thread may still have a client set. Clear the
                // client before we destroy the tcache(s), to avoid us
                // attempting to reference the thread's current tcache
                // once that tcache has been destroyed.
                ThreadLocalData::get().getCurrentClient() = {};
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

// Define our hooks. These are the jemalloc signatures.
static void* cb_alloc(extent_hooks_t* extent_hooks,
                      void* newAddr,
                      size_t size,
                      size_t alignment,
                      bool* zero,
                      bool* commit,
                      unsigned arena);

static bool cb_dalloc(extent_hooks_t* extent_hooks,
                      void* addr,
                      size_t size,
                      bool committed,
                      unsigned arena_ind);

static void cb_destroy(extent_hooks_t* extent_hooks,
                       void* addr,
                       size_t size,
                       bool committed,
                       unsigned arena_ind);

static bool cb_commit(extent_hooks_t* extent_hooks,
                      void* addr,
                      size_t size,
                      size_t offset,
                      size_t length,
                      unsigned arena_ind);

static bool cb_decommit(extent_hooks_t* extent_hooks,
                        void* addr,
                        size_t size,
                        size_t offset,
                        size_t length,
                        unsigned arena_ind);

static bool cb_purge_forced(extent_hooks_t* extent_hooks,
                            void* addr,
                            size_t size,
                            size_t offset,
                            size_t length,
                            unsigned arena_ind);

static bool cb_purge_lazy(extent_hooks_t* extent_hooks,
                          void* addr,
                          size_t size,
                          size_t offset,
                          size_t length,
                          unsigned arena_ind);

static bool cb_split(extent_hooks_t* extent_hooks,
                     void* addr,
                     size_t size,
                     size_t size_a,
                     size_t size_b,
                     bool committed,
                     unsigned arena_ind);

static bool cb_merge(extent_hooks_t* extent_hooks,
                     void* addr_a,
                     size_t size_a,
                     void* addr_b,
                     size_t size_b,
                     bool committed,
                     unsigned arena_ind);

/**
 * Structure hold the hooks which will be given to jemalloc and the orginal
 * hooks from jemalloc.
 *
 * Our hooks are defined to call the jemalloc_hooks functions so must be
 * initialised with the jemalloc hooks.
 */
struct AllocatorHooks {
    AllocatorHooks() = default;

    AllocatorHooks(extent_hooks_t jemalloc_hooks)
        : jemalloc_hooks(jemalloc_hooks),
          couchbase_hooks{cb_alloc,
                          cb_dalloc,
                          cb_destroy,
                          cb_commit,
                          cb_decommit,
                          cb_purge_lazy,
                          cb_purge_forced,
                          cb_split,
                          cb_merge} {
    }

    static AllocatorHooks initialise();

    // Function pointers to the original jemalloc hooks
    const extent_hooks_t jemalloc_hooks = {};
    // Function pointers to our hooks
    extent_hooks_t couchbase_hooks = {};
};

AllocatorHooks AllocatorHooks::initialise() {
    // Read the jemalloc provided hooks using arena 0 (which always exists).
    const std::string_view key = "arena.0.extent_hooks";
    extent_hooks_t* currentHooks = nullptr;
    size_t hooksSize = sizeof(extent_hooks_t*);

    auto rv = je_mallctl(
            key.data(), (void*)&currentHooks, &hooksSize, nullptr, 0);

    if (rv != 0 || currentHooks == nullptr) {
        throw std::runtime_error("AllocatorHooks::initialise failed reading " +
                                 std::string{key} +
                                 " rv:" + std::to_string(rv));
    }
    return {*currentHooks};
}

static AllocatorHooks allocatorHooks = AllocatorHooks::initialise();

static void* cb_alloc(extent_hooks_t* extent_hooks,
                      void* newAddr,
                      size_t size,
                      size_t alignment,
                      bool* zero,
                      bool* commit,
                      unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.alloc(
            extent_hooks, newAddr, size, alignment, zero, commit, arena_ind);
}

static bool cb_dalloc(extent_hooks_t* extent_hooks,
                      void* addr,
                      size_t size,
                      bool committed,
                      unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.dalloc(
            extent_hooks, addr, size, committed, arena_ind);
}

static void cb_destroy(extent_hooks_t* extent_hooks,
                       void* addr,
                       size_t size,
                       bool committed,
                       unsigned arena_ind) {
    allocatorHooks.jemalloc_hooks.destroy(
            extent_hooks, addr, size, committed, arena_ind);
}

static bool cb_commit(extent_hooks_t* extent_hooks,
                      void* addr,
                      size_t size,
                      size_t offset,
                      size_t length,
                      unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.commit(
            extent_hooks, addr, size, offset, length, arena_ind);
}

static bool cb_decommit(extent_hooks_t* extent_hooks,
                        void* addr,
                        size_t size,
                        size_t offset,
                        size_t length,
                        unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.decommit(
            extent_hooks, addr, size, offset, length, arena_ind);
}

static bool cb_purge_forced(extent_hooks_t* extent_hooks,
                            void* addr,
                            size_t size,
                            size_t offset,
                            size_t length,
                            unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.purge_forced(
            extent_hooks, addr, size, offset, length, arena_ind);
}

static bool cb_purge_lazy(extent_hooks_t* extent_hooks,
                          void* addr,
                          size_t size,
                          size_t offset,
                          size_t length,
                          unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.purge_lazy(
            extent_hooks, addr, size, offset, length, arena_ind);
}

static bool cb_split(extent_hooks_t* extent_hooks,
                     void* addr,
                     size_t size,
                     size_t size_a,
                     size_t size_b,
                     bool committed,
                     unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.split(
            extent_hooks, addr, size, size_a, size_b, committed, arena_ind);
}

static bool cb_merge(extent_hooks_t* extent_hooks,
                     void* addr_a,
                     size_t size_a,
                     void* addr_b,
                     size_t size_b,
                     bool committed,
                     unsigned arena_ind) {
    return allocatorHooks.jemalloc_hooks.merge(
            extent_hooks, addr_a, size_a, addr_b, size_b, committed, arena_ind);
}

int makeArena() {
    unsigned arena = 0;
    size_t sz = sizeof(unsigned);
    int rv = je_mallctl("arenas.create", (void*)&arena, &sz, nullptr, 0);

    if (rv != 0) {
        throw std::runtime_error(
                "JEArenaMalloc::makeArena not create arena. rv:" +
                std::to_string(rv));
    }

    std::string key = "arena." + std::to_string(arena) + ".extent_hooks";
    // Give jemalloc our hooks
    extent_hooks_t* hooksp = &allocatorHooks.couchbase_hooks;
    rv = je_mallctl(key.c_str(),
                    nullptr,
                    nullptr,
                    (void*)&hooksp,
                    sizeof(extent_hooks_t*));

    if (rv != 0) {
        throw std::runtime_error("JEArenaMalloc::makeArena failed " + key +
                                 " rv:" + std::to_string(rv));
    }
    return arena;
}

} // namespace cb

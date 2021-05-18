/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/cb_malloc.h>
#include <platform/strerror.h>
#include <platform/platform_thread.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <sys/time.h>
#include <system_error>

#include <phosphor/phosphor.h>

/**
 * The CouchbaseThread class is used to pass information between a thread
 * and the newly created thread.
 */
class CouchbaseThread {
public:
    CouchbaseThread(cb_thread_main_func func_,
                    void* argument_,
                    const char* name_)
        : func(func_),
          argument(argument_) {
        if (name_) {
            name.assign(name_);
            if (name.length() > 15) {
                throw std::logic_error("name exceeds 15 characters");
            }
        }
    }

    void run() {
        PHOSPHOR_INSTANCE.registerThread(name);
        if (!name.empty()) {
            cb_set_thread_name(name.c_str());
        }
        func(argument);
        PHOSPHOR_INSTANCE.deregisterThread();
    }

private:
    cb_thread_main_func func;
    std::string name;
    void* argument;
};

static void *platform_thread_wrap(void *arg)
{
    std::unique_ptr<CouchbaseThread> context(reinterpret_cast<CouchbaseThread*>(arg));
    context->run();
    return nullptr;
}

int cb_create_thread(cb_thread_t *id,
                     cb_thread_main_func func,
                     void *arg,
                     int detached)
{
    // Implemented in terms of cb_create_named_thread; without a name.
    return cb_create_named_thread(id, func, arg, detached, nullptr);
}

int cb_create_named_thread(cb_thread_t *id, cb_thread_main_func func, void *arg,
                           int detached, const char* name)
{
    int ret;
    CouchbaseThread* ctx;
    try {
        ctx = new CouchbaseThread(func, arg, name);
    } catch (std::bad_alloc&) {
        return -1;
    } catch (std::logic_error&) {
        return -1;
    }

    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        delete ctx;
        return -1;
    }

    if (detached &&
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        delete ctx;
        return -1;
    }

    ret = pthread_create(id, &attr, platform_thread_wrap, ctx);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        delete ctx;
    }

    return ret;
}

int cb_join_thread(cb_thread_t id)
{
    if (pthread_self() == id) {
        throw std::runtime_error("cb_join_thread: can't try to join self");
    }

    return pthread_join(id, nullptr);
}

cb_thread_t cb_thread_self()
{
    return pthread_self();
}

int cb_set_thread_name(const char* name)
{
#if defined(__APPLE__)
    // No thread argument (implicit current thread).
    int ret = pthread_setname_np(name);
    if (ret == 0) {
        return 0;
    } else if (errno == ENAMETOOLONG) {
        return 1;
    }
#elif defined(HAVE_PTHREAD_SETNAME_NP)
    errno = 0;
    int ret = pthread_setname_np(pthread_self(), name);
    if (ret == 0) {
        return 0;
    } else if (errno == ERANGE || ret == ERANGE) {
        return 1;
    }
#endif

    return -1;
}

int cb_get_thread_name(char* name, size_t size)
{
#if defined(HAVE_PTHREAD_GETNAME_NP)
    return pthread_getname_np(pthread_self(), name, size);
#else
    return - 1;
#endif
}


bool is_thread_name_supported()
{
#ifdef HAVE_PTHREAD_SETNAME_NP
    return true;
#else
    return false;
#endif
}

void cb_rw_lock_initialize(cb_rwlock_t *rw)
{
    int rv = pthread_rwlock_init(rw, nullptr);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to initialize rw lock");
    }
}

void cb_rw_lock_destroy(cb_rwlock_t *rw)
{
    int rv = pthread_rwlock_destroy(rw);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to destroy rw lock");
    }
}

int cb_rw_reader_enter(cb_rwlock_t *rw)
{
    int result = pthread_rwlock_rdlock(rw);
    if (result != 0) {
        auto err = cb_strerror(result);
        fprintf(stderr, "pthread_rwlock_rdlock returned %d (%s)\n",
                        result, err.c_str());
    }
    return result;
}

int cb_rw_reader_exit(cb_rwlock_t *rw)
{
    int result = pthread_rwlock_unlock(rw);
    if (result != 0) {
        auto err = cb_strerror(result);
        fprintf(stderr, "pthread_rwlock_unlock returned %d (%s)\n",
                        result, err.c_str());
    }
    return result;
}

int cb_rw_writer_enter(cb_rwlock_t *rw)
{
    int result = pthread_rwlock_wrlock(rw);
    if (result != 0) {
        auto err = cb_strerror(result);
        fprintf(stderr, "pthread_rwlock_wrlock returned %d (%s)\n",
                        result, err.c_str());
    }
    return result;
}

int cb_rw_writer_exit(cb_rwlock_t *rw)
{
    int result = pthread_rwlock_unlock(rw);
    if (result != 0) {
        auto err = cb_strerror(result);
        fprintf(stderr, "pthread_rwlock_unlock returned %d (%s)\n",
                        result, err.c_str());
    }
    return result;
}

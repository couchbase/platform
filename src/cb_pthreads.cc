/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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
#include "config.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <sys/time.h>
#include <system_error>

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
        if (!name.empty()) {
            cb_set_thread_name(name.c_str());
        }
        func(argument);
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
    return NULL;
}

int cb_create_thread(cb_thread_t *id,
                     cb_thread_main_func func,
                     void *arg,
                     int detached)
{
    // Implemented in terms of cb_create_named_thread; without a name.
    return cb_create_named_thread(id, func, arg, detached, NULL);
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
        return -1;
    }

    if (detached &&
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
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
    return pthread_join(id, NULL);
}

cb_thread_t cb_thread_self(void)
{
    return pthread_self();
}

int cb_thread_equal(const cb_thread_t a, const cb_thread_t b)
{
    return pthread_equal(a, b);
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


bool is_thread_name_supported(void)
{
#ifdef HAVE_PTHREAD_SETNAME_NP
    return true;
#else
    return false;
#endif
}


void cb_mutex_initialize(cb_mutex_t *mutex)
{
    int rv = pthread_mutex_init(mutex, NULL);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to initialize mutex");
    }
}

void cb_mutex_destroy(cb_mutex_t *mutex)
{
    int rv = pthread_mutex_destroy(mutex);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to destroy mutex");
    }
}

void cb_mutex_enter(cb_mutex_t *mutex)
{
    int rv = pthread_mutex_lock(mutex);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to lock mutex");
    }
}

int cb_mutex_try_enter(cb_mutex_t *mutex) {
    return pthread_mutex_trylock(mutex) == 0 ? 0 : -1;
}

void cb_mutex_exit(cb_mutex_t *mutex)
{
    int rv = pthread_mutex_unlock(mutex);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to release mutex");
    }
}

void cb_cond_initialize(cb_cond_t *cond)
{
    int rv = pthread_cond_init(cond, NULL);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to initialize condition variable");
    }
}

void cb_cond_destroy(cb_cond_t *cond)
{
    int rv = pthread_cond_destroy(cond);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to destroy condition variable");
    }
}

void cb_cond_wait(cb_cond_t *cond, cb_mutex_t *mutex)
{
    int rv = pthread_cond_wait(cond, mutex);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to wait on condition variable");
    }
}

void cb_cond_signal(cb_cond_t *cond)
{
    int rv = pthread_cond_signal(cond);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to signal condition variable");
    }
}

void cb_cond_broadcast(cb_cond_t *cond)
{
    int rv = pthread_cond_broadcast(cond);
    if (rv != 0) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to broadcast condition variable");
    }
}

void cb_cond_timedwait(cb_cond_t *cond, cb_mutex_t *mutex, unsigned int ms)
{
    struct timespec ts;
    struct timeval tp;
    uint64_t wakeup;

    memset(&ts, 0, sizeof(ts));

    /*
     * Unfortunately pthreads don't support relative sleeps so we need
     * to convert back to an absolute time...
     */
    gettimeofday(&tp, NULL);
    wakeup = ((uint64_t)(tp.tv_sec) * 1000) + (tp.tv_usec / 1000) + ms;
    /* Round up for sub ms */
    if ((tp.tv_usec % 1000) > 499) {
        ++wakeup;
    }

    ts.tv_sec = wakeup / 1000;
    wakeup %= 1000;
    ts.tv_nsec = wakeup * 1000000;

    int rv = pthread_cond_timedwait(cond, mutex, &ts);
    if (rv != 0 && rv != ETIMEDOUT) {
        throw std::system_error(rv, std::system_category(),
                                "Failed to do timed wait on condition variable");
    }
}

#ifdef __APPLE__
static const char *get_dll_name(const char *path, char *buffer)
{
    char *ptr = strstr(path, ".dylib");
    if (ptr != NULL) {
        return path;
    }

    strcpy(buffer, path);

    ptr = strstr(buffer, ".so");
    if (ptr != NULL) {
        sprintf(ptr, ".dylib");
        return buffer;
    }

    strcat(buffer, ".dylib");
    return buffer;
}
#else
static const char *get_dll_name(const char *path, char *buffer)
{
    auto* ptr = strstr(path, ".so");
    if (ptr != nullptr) {
        return path;
    }

    strcpy(buffer, path);
    strcat(buffer, ".so");
    return buffer;
}
#endif

cb_dlhandle_t cb_dlopen(const char *library, char **errmsg)
{
    cb_dlhandle_t handle;
    char *buffer = NULL;

    if (library == NULL) {
        handle = dlopen(NULL, RTLD_NOW | RTLD_LOCAL);
    } else {
        handle = dlopen(library, RTLD_NOW | RTLD_LOCAL);
        if (handle == NULL) {
            buffer = reinterpret_cast<char*>(malloc(strlen(library) + 20));
            if (buffer == NULL) {
                if (*errmsg) {
                    *errmsg = strdup("Failed to allocate memory");
                }
                return NULL;
            }

            handle = dlopen(get_dll_name(library, buffer),
                            RTLD_NOW | RTLD_LOCAL);
            free(buffer);
        }
    }

    if (handle == NULL && errmsg != NULL) {
        *errmsg = strdup(dlerror());
    }

    return handle;
}

void *cb_dlsym(cb_dlhandle_t handle, const char *symbol, char **errmsg)
{
    void *ret = dlsym(handle, symbol);
    if (ret == NULL && errmsg) {
        *errmsg = strdup(dlerror());
    }
    return ret;
}

void cb_dlclose(cb_dlhandle_t handle)
{
    dlclose(handle);
}

int platform_set_binary_mode(FILE *fp)
{
    (void)fp;
    return 0;
}

void cb_rw_lock_initialize(cb_rwlock_t *rw)
{
    int rv = pthread_rwlock_init(rw, NULL);
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
        char buffer[64];
        strerror_r(result, buffer, sizeof(buffer));
        fprintf(stderr, "pthread_rwlock_rdlock returned %d (%s)\n",
                        result, buffer);
    }
    return result;
}

int cb_rw_reader_exit(cb_rwlock_t *rw)
{
    int result = pthread_rwlock_unlock(rw);
    if (result != 0) {
        char buffer[64];
        strerror_r(result, buffer, sizeof(buffer));
        fprintf(stderr, "pthread_rwlock_unlock returned %d (%s)\n",
                        result, buffer);
    }
    return result;
}

int cb_rw_writer_enter(cb_rwlock_t *rw)
{
    int result = pthread_rwlock_wrlock(rw);
    if (result != 0) {
        char buffer[64];
        strerror_r(result, buffer, sizeof(buffer));
        fprintf(stderr, "pthread_rwlock_wrlock returned %d (%s)\n",
                        result, buffer);
    }
    return result;
}

int cb_rw_writer_exit(cb_rwlock_t *rw)
{
    int result = pthread_rwlock_unlock(rw);
    if (result != 0) {
        char buffer[64];
        strerror_r(result, buffer, sizeof(buffer));
        fprintf(stderr, "pthread_rwlock_unlock returned %d (%s)\n",
                        result, buffer);
    }
    return result;
}

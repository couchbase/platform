/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc
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
#pragma once
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <stdint.h>
#ifdef __sun
#define CB_DONT_NEED_BYTEORDER 1
#define CB_DONT_NEED_GETHRTIME 1
#include <sys/time.h> /* for hrtime_t */
#endif
#endif

#include <platform/visibility.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    typedef DWORD cb_thread_t;
    typedef CRITICAL_SECTION cb_mutex_t;

#ifdef _MSC_VER
    typedef CONDITION_VARIABLE cb_cond_t;
#else
    /* @TODO make sure that this buffer is big enough!!! */
    typedef struct {
        __int64 blob[64];
    } cb_cond_t;
#endif
    typedef unsigned __int64 hrtime_t;

    /* Unfortunately we don't have stdint.h on windows.. Let's just
     * typedef them here for now.. we need to find a better solution for
     * this!
     */
#if defined(_MSC_VER) && _MSC_VER < 1800
    typedef __int8 int8_t;
    typedef __int16 int16_t;
    typedef __int32 int32_t;
    typedef __int64 int64_t;
    typedef unsigned __int8 uint8_t;
    typedef unsigned __int16 uint16_t;
    typedef unsigned __int32 uint32_t;
    typedef unsigned __int64 uint64_t;
#else
#define CB_DONT_NEED_BYTEORDER 1
#include <stdint.h>
#endif

#else
    typedef pthread_t cb_thread_t;
    typedef pthread_mutex_t cb_mutex_t;
    typedef pthread_cond_t cb_cond_t;

#ifndef __sun
    typedef uint64_t hrtime_t;
#endif

#endif

    /***********************************************************************
     *                     Thread related functions                        *
     **********************************************************************/

    /**
     * Thread function signature.
     */
    typedef void (*cb_thread_main_func)(void *argument);

    /**
     * Create a new thread (in a running state).
     *
     * @param id The thread identifier (returned)
     * @param func The entry point for the newly created thread
     * @param arg Arguments passed to the newly created thread
     * @param detached Set to non-null if the thread should be
     *                 created in a detached state (which you
     *                 can't call cb_join_thread on).
     */
    PLATFORM_PUBLIC_API
    int cb_create_thread(cb_thread_t *id,
                         cb_thread_main_func func,
                         void *arg,
                         int detached);

    /**
     * Wait for a thread to complete
     *
     * @param id The thread identifier to wait for
     */
    PLATFORM_PUBLIC_API
    int cb_join_thread(cb_thread_t id);

    /**
     * Get the id for the running thread
     *
     * @return the id for the running thread
     */
    PLATFORM_PUBLIC_API
    cb_thread_t cb_thread_self(void);

    /**
     * Check if two cb_thread_t objects represent the same thread
     *
     * @param a the first thread
     * @param b the second thread
     * @return nonzero if the two objects represent the same object, 0 otherwise
     */
    PLATFORM_PUBLIC_API
    int cb_thread_equal(const cb_thread_t a, const cb_thread_t b);

    /***********************************************************************
     *                      Mutex related functions                        *
     **********************************************************************/
    /**
     * Initialize a mutex.
     *
     * We don't have <b>any</b> static initializers, so the mutex <b>must</b>
     * be initialized by calling this function before being used.
     *
     * @param mutex the mutex object to initialize
     */
    PLATFORM_PUBLIC_API
    void cb_mutex_initialize(cb_mutex_t *mutex);

    /**
     * Destroy (and release all allocated resources) a mutex.
     *
     * @param mutex the mutex object to destroy
     */
    PLATFORM_PUBLIC_API
    void cb_mutex_destroy(cb_mutex_t *mutex);

    /**
     * Enter a locked section
     *
     * @param mutex the mutex protecting this section
     */
    PLATFORM_PUBLIC_API
    void cb_mutex_enter(cb_mutex_t *mutex);

    /**
     * Exit a locked section
     *
     * @param mutex the mutex protecting this section
     */
    PLATFORM_PUBLIC_API
    void cb_mutex_exit(cb_mutex_t *mutex);

    /***********************************************************************
     *                 Condition variable related functions                *
     **********************************************************************/
    /**
     * Initialize a condition variable
     * @param cond the condition variable to initialize
     */
    PLATFORM_PUBLIC_API
    void cb_cond_initialize(cb_cond_t *cond);

    /**
     * Destroy and release all allocated resources for a condition variable
     * @param cond the condition variable to destroy
     */
    PLATFORM_PUBLIC_API
    void cb_cond_destroy(cb_cond_t *cond);

    /**
     * Wait for a condition variable to be signaled.
     *
     * The mutex must be in a locked state, and this method will release
     * the mutex and wait for the condition variable to be signaled in an
     * atomic operation.
     *
     * The mutex is locked when the method returns.
     *
     * @param cond the condition variable to wait for
     * @param mutex the locked mutex protecting the critical section
     */
    PLATFORM_PUBLIC_API
    void cb_cond_wait(cb_cond_t *cond, cb_mutex_t *mutex);

    /**
     * Wait for a condition variable to be signaled, but give up after a
     * given time.
     *
     * The mutex must be in a locked state, and this method will release
     * the mutex and wait for the condition variable to be signaled in an
     * atomic operation.
     *
     * The mutex is locked when the method returns.
     *
     * @param cond the condition variable to wait for
     * @param mutex the locked mutex protecting the critical section
     * @param ms the number of milliseconds to wait.
     */
    PLATFORM_PUBLIC_API
    void cb_cond_timedwait(cb_cond_t *cond, cb_mutex_t *mutex, unsigned int ms);

    /**
     * Singal a single thread waiting for a condition variable
     *
     * @param cond the condition variable to signal
     */
    PLATFORM_PUBLIC_API
    void cb_cond_signal(cb_cond_t *cond);

    /**
     * Singal all threads waiting for on condition variable
     *
     * @param cond the condition variable to signal
     */
    PLATFORM_PUBLIC_API
    void cb_cond_broadcast(cb_cond_t *cond);


#ifndef CB_DONT_NEED_GETHRTIME
    /**
     * Get a high resolution time
     *
     * @return number of nanoseconds since some arbitrary time in the past
     */
    PLATFORM_PUBLIC_API
    hrtime_t gethrtime(void);
#endif

#ifndef CB_DONT_NEED_BYTEORDER
    PLATFORM_PUBLIC_API
    uint64_t ntohll(uint64_t);

    PLATFORM_PUBLIC_API
    uint64_t htonll(uint64_t);
#endif

    typedef void *cb_dlhandle_t;

    PLATFORM_PUBLIC_API
    cb_dlhandle_t cb_dlopen(const char *library, char **errmsg);

    PLATFORM_PUBLIC_API
    void *cb_dlsym(cb_dlhandle_t handle, const char *symbol, char **errmsg);

    PLATFORM_PUBLIC_API
    void cb_dlclose(cb_dlhandle_t handle);

#ifdef WIN32
    struct iovec {
        size_t iov_len;
        void *iov_base;
    };

    struct msghdr {
        void *msg_name;         /* Socket name */
        int msg_namelen;       /* Length of name */
        struct iovec *msg_iov; /* Data blocks */
        int msg_iovlen;        /* Number of blocks */
    };

    PLATFORM_PUBLIC_API
    int sendmsg(SOCKET sock, const struct msghdr *msg, int flags);

    /**
     * Initialize the winsock library
     */
    PLATFORM_PUBLIC_API
    void cb_initialize_sockets(void);
#else

#define cb_initialize_sockets()

#endif

#ifdef __cplusplus
}
#endif

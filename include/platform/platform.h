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

#include <platform/dynamic.h>
#include <platform/visibility.h>

/* PLATFORM_PUBLIC_API
 *
 * Used for functions which are part of the public API of platform.
 * "Inside" platform (i.e. when compiling platform.so) they will export the
 * symbol
 * "Outside" platform (i.e. when compiling code which wants to link to
 * platform.so) they will allow the symbol to be imported from platform.so
 */
#if defined(platform_EXPORTS)
#define PLATFORM_PUBLIC_API EXPORT_SYMBOL
#else
#define PLATFORM_PUBLIC_API IMPORT_SYMBOL
#endif

#include <platform/cbassert.h>

#ifdef WIN32
/* Include winsock2.h before windows.h to avoid winsock.h to be included */
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

#include <time.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __sun
#include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    typedef DWORD cb_thread_t;
    typedef CRITICAL_SECTION cb_mutex_t;
    typedef CONDITION_VARIABLE cb_cond_t;
    typedef long ssize_t;
    typedef unsigned __int64 hrtime_t;
#define CB_DONT_NEED_BYTEORDER 1
#define DIRECTORY_SEPARATOR_CHARACTER '\\'

#else

#define DIRECTORY_SEPARATOR_CHARACTER '/'

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
     * Create a new thread (in a running state), with a name.
     *
     * @param id The thread identifier (returned)
     * @param func The entry point for the newly created thread
     * @param arg Arguments passed to the newly created thread
     * @param detached Set to non-null if the thread should be
     *                 created in a detached state (which you
     *                 can't call cb_join_thread on).
     * @param name Name of the thread. Maximum of 16 characters in length,
     *             including terminating '\0'. (Note: name ignored on Windows).
     */
    PLATFORM_PUBLIC_API
    int cb_create_named_thread(cb_thread_t *id, cb_thread_main_func func,
                               void *arg, int detached, const char* name);

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

    /**
     * Sets the current threads' name.
     *
     * @param name New value for the current threads' name. If non-NULL,
     *             maximum of 16 characters in length.
     */
    PLATFORM_PUBLIC_API
    void cb_set_thread_name(const char* name);

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
     * Try to enter a locked section
     *
     * @param mutex the mutex protecting this section
     * @return 0 if the mutex was obtained, -1 otherwise
     */
    PLATFORM_PUBLIC_API
    int cb_mutex_try_enter(cb_mutex_t *mutex);

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

    /**
     * Get the period of the high resolution time clock.
     *
     * @return Period of the clock in nanoseconds.
     */
    PLATFORM_PUBLIC_API
    hrtime_t gethrtime_period(void);

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

    PLATFORM_PUBLIC_API
    void usleep(unsigned int useconds);

    PLATFORM_PUBLIC_API
    int gettimeofday(struct timeval *tv, void *tz);
#else

#define cb_initialize_sockets()

#endif

    /*
     * Set mode to binary
     */
    PLATFORM_PUBLIC_API
    int platform_set_binary_mode(FILE *fp);

    /*
        return a monotonically increasing value with a seconds frequency.
    */
    PLATFORM_PUBLIC_API
    uint64_t cb_get_monotonic_seconds(void);

    /*
        obtain a timeval structure containing the current time since EPOCH.
    */
    PLATFORM_PUBLIC_API
    int cb_get_timeofday(struct timeval *tv);

    /*
        set an offset so that cb_get_timeofday returns an offsetted time.
        This is intended for testing of time jumps.
    */
    PLATFORM_PUBLIC_API
    void cb_set_timeofday_offset(uint64_t offset);

    /**
     * Some of our platforms complain on not using mkstemp. Instead of
     * having the test in all programs we're just going to use this
     * method instead.
     *
     * @param pattern The input pattern for the filename. It should end
     *                with six X's that will be replaced with the unique
     *                filename. The file will be created.
     * @return pattern on success, NULL upon failure. Check errno for
     *                 the reason.
     */
    PLATFORM_PUBLIC_API
    char *cb_mktemp(char *pattern);

    /**
     * Convert time_t to a structure
     *
     * @param clock the input value
     * @param result the output value
     * @return 0 for success, -1 on failure
     */
    PLATFORM_PUBLIC_API
    int cb_gmtime_r(const time_t *clock, struct tm *result);


    /**
     * Convert a time value with adjustments for the local time zone
     *
     * @param clock the input value
     * @param result the output value
     * @return 0 for success, -1 on failure
     */
    PLATFORM_PUBLIC_API
    int cb_localtime_r(const time_t *clock, struct tm *result);

    /**
     * Definition of a process identifier
     */
    typedef int cb_pid_t;

    /**
     * Get the process identifier for the running process
     */
    PLATFORM_PUBLIC_API
    cb_pid_t cb_getpid(void);


#ifdef __cplusplus
}
#endif

/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc.
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

/*
 * Header including threading portability declarations
 */
#pragma once

#include <platform/visibility.h>

#ifdef WIN32

#include <folly/portability/Windows.h>

typedef DWORD cb_thread_t;
typedef SRWLOCK cb_rwlock_t;
#else
#include <pthread.h>
typedef pthread_t cb_thread_t;
typedef pthread_rwlock_t cb_rwlock_t;
#endif // WIN32

/***********************************************************************
 *                     Thread related functions                        *
 **********************************************************************/

/**
 * Thread function signature.
 */
typedef void (*cb_thread_main_func)(void* argument);

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
int cb_create_thread(cb_thread_t* id,
                     cb_thread_main_func func,
                     void* arg,
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
int cb_create_named_thread(cb_thread_t* id,
                           cb_thread_main_func func,
                           void* arg,
                           int detached,
                           const char* name);

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
 * Sets the current threads' name.
 *
 * This method tries to set the current threads name by using
 * pthread_setname_np (which means that it is not implemented
 * on windows)
 *
 * @param name New value for the current threads' name
 * @return 0 for success, 1 if the specified name is too long and
 *         -1 if an error occurred
 */
PLATFORM_PUBLIC_API
int cb_set_thread_name(const char* name);

/**
 * Try to get the name of the current thread
 *
 * @param name destination buffer
 * @param size size of destination buffer
 * @return 0 for success
 *         -1 if an error occurred
 */
PLATFORM_PUBLIC_API
int cb_get_thread_name(char* name, size_t size);

/**
 * Does the underlying platform support setting thread names
 */
PLATFORM_PUBLIC_API
bool is_thread_name_supported(void);

/***********************************************************************
 *                 Reader/Writer lock  related functions               *
 **********************************************************************/

/**
 * Initialize a read/write lock
 */
PLATFORM_PUBLIC_API
void cb_rw_lock_initialize(cb_rwlock_t* rw);

/**
 * Destroy a read/write lock
 */
PLATFORM_PUBLIC_API
void cb_rw_lock_destroy(cb_rwlock_t* rw);

/*
 * Obtain reader access to the rw_lock
 * Return 0 if succesfully entered the critical section.
 */
PLATFORM_PUBLIC_API
int cb_rw_reader_enter(cb_rwlock_t* rw);

/*
 * Exit the lock if previously entered as a reader.
 * Return 0 if succesfully exited the critical section.
 */
PLATFORM_PUBLIC_API
int cb_rw_reader_exit(cb_rwlock_t* rw);

/*
 * Obtain writer access to the rw_lock
 * Return 0 if succesfully entered the critical section.
 */
PLATFORM_PUBLIC_API
int cb_rw_writer_enter(cb_rwlock_t* rw);

/*
 * Exit the lock if previously entered as a writer.
 * Return 0 if succesfully exited the critical section.
 */
PLATFORM_PUBLIC_API
int cb_rw_writer_exit(cb_rwlock_t* rw);

/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/*
 * Header including threading portability declarations
 */
#pragma once

#ifdef WIN32

#include <folly/portability/Windows.h>

typedef DWORD cb_thread_t;
typedef SRWLOCK cb_rwlock_t;
#else
#include <pthread.h>
typedef pthread_t cb_thread_t;
typedef pthread_rwlock_t cb_rwlock_t;
#endif // WIN32

#include <string>

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
int cb_join_thread(cb_thread_t id);

/**
 * Get the id for the running thread
 *
 * @return the id for the running thread
 */
cb_thread_t cb_thread_self();

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
int cb_set_thread_name(const char* name);

/**
 * Try to get the name of the current thread
 *
 * @param name destination buffer
 * @param size size of destination buffer
 * @return 0 for success
 *         -1 if an error occurred
 */
int cb_get_thread_name(char* name, size_t size);

/**
 * Get the name of a given tread
 *
 * @param tid The thread to get the name for
 * @return The name of the thread (or std::to_string(tid))
 */
std::string cb_get_thread_name(cb_thread_t tid);

/**
 * Does the underlying platform support setting thread names
 */
bool is_thread_name_supported();

/***********************************************************************
 *                 Reader/Writer lock  related functions               *
 **********************************************************************/

/**
 * Initialize a read/write lock
 */
void cb_rw_lock_initialize(cb_rwlock_t* rw);

/**
 * Destroy a read/write lock
 */
void cb_rw_lock_destroy(cb_rwlock_t* rw);

/*
 * Obtain reader access to the rw_lock
 * Return 0 if succesfully entered the critical section.
 */
int cb_rw_reader_enter(cb_rwlock_t* rw);

/*
 * Exit the lock if previously entered as a reader.
 * Return 0 if succesfully exited the critical section.
 */
int cb_rw_reader_exit(cb_rwlock_t* rw);

/*
 * Obtain writer access to the rw_lock
 * Return 0 if succesfully entered the critical section.
 */
int cb_rw_writer_enter(cb_rwlock_t* rw);

/*
 * Exit the lock if previously entered as a writer.
 * Return 0 if succesfully exited the critical section.
 */
int cb_rw_writer_exit(cb_rwlock_t* rw);

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

typedef SRWLOCK cb_rwlock_t;
#else
#include <pthread.h>
typedef pthread_rwlock_t cb_rwlock_t;
#endif // WIN32

#include <functional>
#include <string>
#include <thread>

/***********************************************************************
 *                     Thread related functions                        *
 **********************************************************************/

/**
 * Create a thread which runs the provided function (and tries to set the
 * thread name to the provided name)
 *
 * @param main The method to run within the thread object
 * @param name The name for the thread (not const as it gets moved
 *             over to the running thread to clean up)
 * @return The (running) thread object
 */
std::thread create_thread(std::function<void()> main, std::string name);

/// We'll only support a thread name up to 32 characters
constexpr size_t MaxThreadNameLength = 32;

/**
 * Sets the current threads' name.
 *
 * @param name New value for the current threads' name
 * @return true for success, false otherwise
 * @throws std::logic_error if the thread name exceeds the max length
 */
bool cb_set_thread_name(std::string_view name);

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

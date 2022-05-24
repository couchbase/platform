/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <platform/platform_thread.h>

#ifdef WIN32
#include <folly/portability/Windows.h>
using cb_rwlock_t = SRWLOCK;
#else
#include <pthread.h>
using cb_rwlock_t = pthread_rwlock_t;
#endif // WIN32

#include <stdexcept>
#include <string>

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

namespace cb {

class ReaderLock;

class WriterLock;

/**
 *   Reader/Write lock abstraction for platform provided cb_rw_lock
 *
 *   The lock allows many readers but mutual exclusion with a writer.
 */
class RWLock {
public:
    RWLock() {
        cb_rw_lock_initialize(&rwlock);
    }

    ~RWLock() {
        cb_rw_lock_destroy(&rwlock);
    }

    /**
     * Returns a `ReaderLock` reference which implements BasicLockable
     * to allow managing the reader lock with a std::lock_guard
     */
    ReaderLock& reader();

    operator ReaderLock&() {
        return reader();
    }

    /**
     * Returns a `WriterLock` reference which implements BasicLockable
     * to allow managing the writer lock with a std::lock_guard
     */
    WriterLock& writer();

    operator WriterLock&() {
        return writer();
    }

    /// Allows use of std::shared_lock RAII wrapper
    void lock_shared() {
        readerLock();
    }

    /// Allows use of std::shared_lock RAII wrapper
    void unlock_shared() {
        readerUnlock();
    }

    /// Allows use of std::unique_lock RAII wrapper
    void lock() {
        writerLock();
    }

    /// Allows use of std::unique_lock RAII wrapper
    void unlock() {
        writerUnlock();
    }

protected:
    void readerLock() {
        auto locked = cb_rw_reader_enter(&rwlock);
        if (locked != 0) {
            throw std::runtime_error(std::to_string(locked) +
                                     " returned by cb_rw_reader_enter()");
        }
    }

    void readerUnlock() {
        int unlocked = cb_rw_reader_exit(&rwlock);
        if (unlocked != 0) {
            throw std::runtime_error(std::to_string(unlocked) +
                                     " returned by cb_rw_reader_exit()");
        }
    }

    void writerLock() {
        int locked = cb_rw_writer_enter(&rwlock);
        if (locked != 0) {
            throw std::runtime_error(std::to_string(locked) +
                                     " returned by cb_rw_writer_enter()");
        }
    }

    void writerUnlock() {
        int unlocked = cb_rw_writer_exit(&rwlock);
        if (unlocked != 0) {
            throw std::runtime_error(std::to_string(unlocked) +
                                     " returned by cb_rw_writer_exit()");
        }
    }

private:
    cb_rwlock_t rwlock;

    RWLock(const RWLock&) = delete;
    void operator=(const RWLock&) = delete;
};

/**
 * BasicLockable abstraction around the reader lock of an RWLock
 */
class ReaderLock : public RWLock {
public:
    void lock() {
        readerLock();
    }

    void unlock() {
        readerUnlock();
    }
};

/**
 * BasicLockable abstraction around the writer lock of an RWLock
 */
class WriterLock : public RWLock {
public:
    void lock() {
        writerLock();
    }

    void unlock() {
        writerUnlock();
    }
};

inline ReaderLock& RWLock::reader() {
    return static_cast<ReaderLock&>(*this);
}

inline WriterLock& RWLock::writer() {
    return static_cast<WriterLock&>(*this);
}

} // namespace cb

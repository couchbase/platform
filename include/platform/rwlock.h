/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
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

#include <platform/platform_thread.h>

#include <stdexcept>
#include <string>

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

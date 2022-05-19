/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/system/ThreadName.h>
#include <platform/cb_malloc.h>
#include <platform/platform_thread.h>
#include <platform/strerror.h>

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

cb_thread_t cb_thread_self()
{
    return pthread_self();
}

bool cb_set_thread_name(std::string_view name) {
    if (name.size() > MaxThreadNameLength) {
        throw std::logic_error("cb_set_thread_name: thread name too long");
    }

    return folly::setThreadName(name);
}

std::string cb_get_thread_name() {
    auto name = folly::getCurrentThreadName();
    if (name.has_value()) {
        return *name;
    }

    return std::to_string(uint64_t(cb_thread_self()));
}

bool is_thread_name_supported() {
    return folly::canSetCurrentThreadName();
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

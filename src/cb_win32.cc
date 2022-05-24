/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/getopt.h>
#include <platform/rwlock.h>
#include <win32/getopt.h>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>
#include <vector>

void cb_rw_lock_initialize(cb_rwlock_t* rw) {
    InitializeSRWLock(rw);
}

void cb_rw_lock_destroy(cb_rwlock_t* rw) {
    (void)rw;
    // Nothing todo on windows
}

int cb_rw_reader_enter(cb_rwlock_t* rw) {
    AcquireSRWLockShared(rw);
    return 0;
}

int cb_rw_reader_exit(cb_rwlock_t* rw) {
    ReleaseSRWLockShared(rw);
    return 0;
}

int cb_rw_writer_enter(cb_rwlock_t* rw) {
    AcquireSRWLockExclusive(rw);
    return 0;
}

int cb_rw_writer_exit(cb_rwlock_t* rw) {
    ReleaseSRWLockExclusive(rw);
    return 0;
}

// Wrapper into cb::getopt (which we now unit tests on all platforms)
char* optarg;
int opterr;
int optind = 1;
int optopt;

int getopt_long(int argc,
                char** argv,
                const char* optstring,
                const struct option* longopts,
                int* longindex) {
    cb::getopt::optind = optind;
    cb::getopt::opterr = opterr;
    cb::getopt::optopt = optopt;
    auto ret = cb::getopt::getopt_long(
            argc,
            argv,
            optstring,
            reinterpret_cast<const cb::getopt::option*>(longopts),
            longindex);
    optarg = cb::getopt::optarg;
    opterr = cb::getopt::opterr;
    optind = cb::getopt::optind;
    optopt = cb::getopt::optopt;
    return ret;
}

int getopt(int argc, char** argv, const char* optstring) {
    cb::getopt::optind = optind;
    cb::getopt::opterr = opterr;
    cb::getopt::optopt = optopt;
    auto ret = cb::getopt::getopt(argc, argv, optstring);
    optarg = cb::getopt::optarg;
    opterr = cb::getopt::opterr;
    optind = cb::getopt::optind;
    optopt = cb::getopt::optopt;
    return ret;
}

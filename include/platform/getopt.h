/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

/**
 * This file contains a getopt implementation which is only used
 * on Windows (but it is built on all platforms to make it easier
 * to test ;)
 */


namespace cb::getopt {

const int no_argument = 0;
const int required_argument = 1;
const int optional_argument = 2;

struct option {
    const char* name;
    int has_arg;
    int* flag;
    int val;
};

extern char* optarg;
extern int opterr;
extern int optind;
extern int optopt;

extern int getopt_long(int argc,
                       char** argv,
                       const char* optstring,
                       const struct option* longopts,
                       int* longindex);

extern int getopt(int argc, char** argv, const char* optstring);

/**
 * This is for unit tests only and used to reset the internal state
 * of the library
 */
void reset();

/**
 * This is used for unit tests to mute the unit tests from writing error
 * messages to stderr
 */
void mute_stderr();
} // namespace cb::getopt

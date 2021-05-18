/*
 *     Copyright 2013-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#define no_argument 0
#define required_argument 1
#define optional_argument 2

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

#ifdef __cplusplus
}
#endif

/*
 *     Copyright 2013-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/getopt.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

char* cb::getopt::optarg;
int cb::getopt::opterr;
int cb::getopt::optind = 1;
int cb::getopt::optopt;

static bool silent = false;

static int parse_longopt(int argc,
                         char** argv,
                         const cb::getopt::option* longopts,
                         int* longindex) {
    std::string name{argv[cb::getopt::optind] + 2};
    if (name.empty()) {
        ++cb::getopt::optind;
        return -1;
    }

    auto idx = name.find('=');
    bool resized = false;
    if (idx != std::string::npos) {
        name.resize(idx);
        resized = true;
    }

    cb::getopt::optarg = nullptr;
    for (const auto* p = longopts; p != nullptr && p->name; ++p) {
        if (name == p->name) {
            // This is it :)
            if (p->has_arg == cb::getopt::required_argument) {
                // we need a value!
                if (resized) {
                    // the value was part of the string:
                    cb::getopt::optarg = argv[cb::getopt::optind] + 2 + idx + 1;
                } else if (++cb::getopt::optind == argc) {
                    if (!silent) {
                        fprintf(stderr,
                                "%s: option requires an argument -- %s\n",
                                argv[0],
                                name.c_str());
                    }
                    return '?';
                } else {
                    // The option follows this entry
                    cb::getopt::optarg = argv[cb::getopt::optind];
                }
            } else if (p->has_arg == cb::getopt::optional_argument) {
                if (resized) {
                    // the value was part of the string:
                    // (the +3 is for the leading dashes and the equal sign
                    cb::getopt::optarg = argv[cb::getopt::optind] + idx + 3;
                }
            }
            ++cb::getopt::optind;
            return p->val;
        }
    }

    // Not found, we should increase optind too,
    // to continue getopt_long().
    cb::getopt::optind++;
    return '?';
}

int cb::getopt::getopt_long(int argc,
                            char** argv,
                            const char* optstring,
                            const cb::getopt::option* longopts,
                            int* longindex) {
    if (cb::getopt::optind + 1 > argc) {
        // EOF
        return -1;
    }

    if (argv[cb::getopt::optind][0] != '-') {
        return -1;
    }

    if (argv[cb::getopt::optind][1] == '-') {
        // this is a long option
        return parse_longopt(argc, argv, longopts, longindex);
    } else if (argv[cb::getopt::optind][2] != '\0') {
        if (!silent) {
            fprintf(stderr,
                    "You can't specify multiple options with this "
                    "implementation\n");
        }
        return '?';
    } else {
        // this is a short option
        const char* p = strchr(optstring, argv[cb::getopt::optind][1]);
        int idx = cb::getopt::optind;
        cb::getopt::optind++;

        if (p == nullptr) {
            return '?';
        }

        if (*(p + 1) == ':') {
            cb::getopt::optarg = argv[cb::getopt::optind];
            cb::getopt::optind++;
            if (cb::getopt::optarg == nullptr || cb::getopt::optind > argc) {
                if (!silent) {
                    fprintf(stderr,
                            "%s: option requires an argument -- %s\n",
                            argv[0],
                            argv[idx] + 1);
                }
                return '?';
            }
        } else {
            cb::getopt::optarg = nullptr;
        }
        return argv[idx][1];
    }
}

int cb::getopt::getopt(int argc, char** argv, const char* optstring) {
    return cb::getopt::getopt_long(argc, argv, optstring, nullptr, nullptr);
}

void cb::getopt::reset() {
    cb::getopt::optarg = nullptr;
    cb::getopt::opterr = 0;
    cb::getopt::optind = 1;
    cb::getopt::optopt = 0;
}

void cb::getopt::mute_stderr() {
    silent = true;
}

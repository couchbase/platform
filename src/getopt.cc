/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc.
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
#define BUILDING_LIBPLATFORM 1
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

using namespace cb::getopt;

static int parse_longopt(int argc,
                         char** argv,
                         const option* longopts,
                         int* longindex) {
    std::string name{argv[optind] + 2};
    if (name.empty()) {
        ++optind;
        return -1;
    }

    auto idx = name.find('=');
    bool resized = false;
    if (idx != std::string::npos) {
        name.resize(idx);
        resized = true;
    }

    optarg = nullptr;
    for (const auto* p = longopts; p != nullptr && p->name; ++p) {
        if (name == p->name) {
            // This is it :)
            if (p->has_arg == required_argument) {
                // we need a value!
                if (resized) {
                    // the value was part of the string:
                    optarg = argv[optind] + 2 + idx + 1;
                } else if (++optind == argc) {
                    if (!silent) {
                        fprintf(stderr,
                                "%s: option requires an argument -- %s\n",
                                argv[0],
                                name.c_str());
                    }
                    return '?';
                } else {
                    // The option follows this entry
                    optarg = argv[optind];
                }
            } else if (p->has_arg == optional_argument) {
                if (resized) {
                    // the value was part of the string:
                    // (the +3 is for the leading dashes and the equal sign
                    optarg = argv[optind] + idx + 3;
                }
            }
            ++optind;
            return p->val;
        }
    }

    // Not found, we should increase optind too,
    // to continue getopt_long().
    optind++;
    return '?';
}

int cb::getopt::getopt_long(int argc,
                            char** argv,
                            const char* optstring,
                            const option* longopts,
                            int* longindex) {
    if (optind + 1 > argc) {
        // EOF
        return -1;
    }

    if (argv[optind][0] != '-') {
        return -1;
    }

    if (argv[optind][1] == '-') {
        // this is a long option
        return parse_longopt(argc, argv, longopts, longindex);
    } else if (argv[optind][2] != '\0') {
        if (!silent) {
            fprintf(stderr,
                    "You can't specify multiple options with this "
                    "implementation\n");
        }
        return '?';
    } else {
        // this is a short option
        const char* p = strchr(optstring, argv[optind][1]);
        int idx = optind;
        optind++;

        if (p == nullptr) {
            return '?';
        }

        if (*(p + 1) == ':') {
            optarg = argv[optind];
            optind++;
            if (optarg == nullptr || optind > argc) {
                if (!silent) {
                    fprintf(stderr,
                            "%s: option requires an argument -- %s\n",
                            argv[0],
                            argv[idx] + 1);
                }
                return '?';
            }
        } else {
            optarg = nullptr;
        }
        return argv[idx][1];
    }
}

int cb::getopt::getopt(int argc, char** argv, const char* optstring) {
    return getopt_long(argc, argv, optstring, nullptr, nullptr);
}

void cb::getopt::reset() {
    optarg = nullptr;
    opterr = 0;
    optind = 1;
    optopt = 0;
}

void cb::getopt::mute_stderr() {
    silent = true;
}

/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <boost/filesystem.hpp>
#include <getopt.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

[[noreturn]] void usage(const boost::filesystem::path name) {
    std::cerr << "Usage: " << name.filename().string() << " [options]\n"
              << "    --lockfile filename" << std::endl
              << "    --exitcode exitcode" << std::endl;
    exit(5);
}

void waitWhileLockFile(const boost::filesystem::path lockfile) {
    while (exists(lockfile)) {
        std::this_thread::sleep_for(std::chrono::microseconds{10});
    }
}

int main(int argc, char** argv) {
    std::vector<option> options = {
            {"lockfile", required_argument, nullptr, 'l'},
            {"exitcode", required_argument, nullptr, 'e'},
            {nullptr, 0, nullptr, 0}};
    std::string lockfile;

    int c;
    int exitcode = EXIT_SUCCESS;

    while ((c = getopt_long(argc, argv, "", options.data(), nullptr)) != EOF) {
        switch (c) {
        case 'l':
            lockfile.assign(optarg);
            break;
        case 'e':
            exitcode = atoi(optarg);
            break;
        default:
            usage(argv[0]);
        }
    }

    if (lockfile.empty() || exitcode == -1) {
        usage(argv[0]);
    }

    waitWhileLockFile(lockfile);
    return exitcode;
}

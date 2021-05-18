/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/**
 * This program/test is designed to block waiting for a ctrl+c signal. If the
 * signal handler has been setup properly then program should return 0 when
 * SIGINT is triggered. If it has not been setup properly then the program
 * will be forced to exit and will return 1 instead.
 */

#include <atomic>
#include <iostream>
#include <thread>

#include <platform/interrupt.h>

static std::atomic<bool> go{false};

void handler() {
    go = true;
}

int main(int argc, char** argv) {
    std::cerr << "Registering SIGINT handler\n";
    cb::console::set_sigint_handler(handler);

    std::cerr << "Busy waiting for signal\n";
    while (!go) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cerr << "SIGINT detected!\n";
    return 0;
}

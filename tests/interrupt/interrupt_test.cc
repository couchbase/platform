/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

#include <platform/interrupt.h>
#include <atomic>
#include <system_error>

#ifdef WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

namespace cb::console {

static std::atomic<interrupt_handler> sigint_handler{nullptr};

#ifdef WIN32
static bool WINAPI HandlerRoutine(_In_ DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        auto si_handler = sigint_handler.load();
        if (si_handler) {
            si_handler();
        }
        return true;
    }

    return false;
}
#else
static void sigint_handler_wrapper(int s) {
    auto si_handler = sigint_handler.load();
    if (si_handler) {
        si_handler();
    }
}
#endif

void set_sigint_handler(interrupt_handler h) {
    bool initialised = sigint_handler.load() != nullptr;
    sigint_handler = h;

    if (!initialised) {
#ifdef WIN32
        if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true)) {
            throw std::system_error(GetLastError(),
                                    std::system_category(),
                                    "cb::console::set_sigint_handler: Couldn't "
                                    "register ConsoleCtrlHandler");
        }
#else
        // Register our SIGINT handler
        struct sigaction sigIntHandler {};
        sigIntHandler.sa_handler = sigint_handler_wrapper;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;
        if (sigaction(SIGINT, &sigIntHandler, nullptr) != 0) {
            throw std::system_error(errno,
                                    std::system_category(),
                                    "cb::console::set_sigint_handler: Failed "
                                    "to register sigaction");
        }
#endif
    }
}

void clear_sigint_handler() {
    sigint_handler = nullptr;
}
} // namespace cb::console

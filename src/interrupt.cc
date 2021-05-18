/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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

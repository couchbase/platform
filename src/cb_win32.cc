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
#include <platform/platform_thread.h>
#include <win32/getopt.h>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>
#include <vector>

cb_thread_t cb_thread_self(void) {
    return GetCurrentThreadId();
}

class ThreadNameSupport {
public:
    bool setName(std::string name) {
        if (!supported) {
            return false;
        }

        // Windows doesn't really have this restriction, but we have a unit
        // test which wants this to fail (because it does on Posix)...
        if (name.size() > MaxThreadNameLength) {
            throw std::logic_error("cb_set_thread_name: thread name too long");
        }

        const auto thread_name = to_wstring(name);
        if (SUCCEEDED(set(GetCurrentThread(), thread_name.c_str()))) {
            return true;
        }

        return false;
    }

    std::optional<std::string> getName(HANDLE tid) {
        if (!supported) {
            return {};
        }

        PWSTR data;
        if (SUCCEEDED(get(tid, &data))) {
            auto str = to_string(std::wstring{data});
            LocalFree(data);
            return {str};
        }

        return {};
    }

    bool isSupported() const {
        return supported;
    }

    static ThreadNameSupport& instance() {
        static ThreadNameSupport instance;
        return instance;
    }

protected:
    ThreadNameSupport() {
        auto module = GetModuleHandle(TEXT("kernel32.dll"));
        if (module == nullptr) {
            return;
        }
        set = (SetFunc)GetProcAddress(module, "SetThreadDescription");
        get = (GetFunc)GetProcAddress(module, "GetThreadDescription");
        supported = (set != nullptr && get != nullptr);
    }

    static std::wstring to_wstring(const std::string& s) {
        // determine the required size
        auto len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring buffer;
        buffer.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buffer.data(), len);
        return buffer;
    }

    static std::string to_string(const std::wstring& ws) {
        auto len = WideCharToMultiByte(
                CP_ACP, 0, ws.data(), -1, nullptr, 0, nullptr, nullptr);

        std::string buffer;
        buffer.resize(len);
        WideCharToMultiByte(CP_ACP,
                            0,
                            ws.data(),
                            -1,
                            buffer.data(),
                            (int)len,
                            nullptr,
                            nullptr);

        return buffer;
    }

    bool supported = false;
    typedef HRESULT(WINAPI* SetFunc)(HANDLE, PCWSTR);
    typedef HRESULT(WINAPI* GetFunc)(HANDLE, PWSTR*);
    SetFunc set;
    GetFunc get;
};

bool cb_set_thread_name(std::string_view name) {
    return ThreadNameSupport::instance().setName(
            std::string{name.data(), name.size()});
}

std::string cb_get_thread_name(cb_thread_t tid) {
    auto handle = OpenThread(READ_CONTROL, false, tid);
    if (handle == INVALID_HANDLE_VALUE) {
        return std::to_string(tid);
    }
    auto nm = ThreadNameSupport::instance().getName(handle);
    CloseHandle(handle);
    if (nm.has_value()) {
        return nm.value();
    }

    return std::to_string(tid);
}

std::string cb_get_thread_name() {
    return cb_get_thread_name(cb_thread_self());
}

bool is_thread_name_supported() {
    return ThreadNameSupport::instance().isSupported();
}

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

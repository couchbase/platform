/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/process_monitor.h>

#include <folly/Synchronized.h>
#include <nlohmann/json.hpp>
#include <platform/platform_thread.h>
#include <platform/string_hex.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#endif

class ExitCodeImpl : public ProcessMonitor::ExitCode {
public:
    ExitCodeImpl() : code(EXIT_FAILURE) {
    }
    explicit ExitCodeImpl(uint32_t code) : code(code) {
    }
    std::string to_string() const override {
        if (isSuccess()) {
            return "Success";
        }

#ifdef WIN32
        if (code == EXIT_FAILURE) {
            return "Failure";
        }

        return "Failure: " + cb::to_hex(code);
#else
        if (WIFEXITED(code)) {
            if (WEXITSTATUS(code) == EXIT_FAILURE) {
                return "Failure";
            }

            return "Failure: " + cb::to_hex(uint8_t(WEXITSTATUS(code)));
        }
        return "Crashed: " + cb::to_hex(uint8_t(WTERMSIG(code)));
#endif
    }

    nlohmann::json to_json() const override {
#ifdef WIN32
        return nlohmann::json{{ "WEXITSTATUS", code }};
#else
        return nlohmann::json{{"WIFEXITED", WIFEXITED(code)},
                              {"WEXITSTATUS", WEXITSTATUS(code)},
                              {"WIFSIGNALED", WIFSIGNALED(code)},
                              {"WTERMSIG", WTERMSIG(code)},
                              {"WTERMSIGNAL", strsignal(WTERMSIG(code))},
                              {"WCOREDUMP", WCOREDUMP(code)}};
#endif
    }

    bool isSuccess() const override {
#ifdef WIN32
        return code == EXIT_SUCCESS;
#else
        return (WIFEXITED(code) && WEXITSTATUS(code) == EXIT_SUCCESS);
#endif
    }

    uint32_t code;
};

/**
 * The class responsible for monitoring a process.
 *
 * Detecting if a process is alive or not differs between a child process
 * and other processes so there is two different subclasses handling the
 * logic for that
 */
class ProcessMonitorImpl {
public:
#ifdef WIN32
    virtual ~ProcessMonitorImpl() {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }
#else
    virtual ~ProcessMonitorImpl() = default;
#endif

    void terminateMonitorThread() {
        active = false;
        shutdown_cv.notify_all();
        thread.join();
    }

    nlohmann::json describe() const {
        return description;
    }

    ProcessMonitor::ExitCode& getExitCode() {
        auto locked = state.lock();
        if (isRunning(locked)) {
            throw std::logic_error(
                    "ProcessMonitor: getResultCode() process still "
                    "running");
        }
        return *locked->processExitCode;
    }

    bool isRunning() {
        auto locked = state.lock();
        return isRunning(locked);
    }

    void setTimeoutHander(std::chrono::seconds seconds,
                          ProcessMonitor::TimeoutHandler handler) {
        auto locked = state.lock();
        locked->timeout = std::chrono::steady_clock::now() + seconds;
        locked->timeoutHandler = handler;
    }

    void terminate(bool allowGraceful = true) {
        if (!isRunning()) {
            // already stopped
            return;
        }

#ifdef WIN32
        if (!TerminateProcess(handle, EXIT_FAILURE)) {
            throw std::system_error(GetLastError(),
                                    std::system_category(),
                                    "ProcessMonitorImpl::terminate: "
                                    "TerminateProcess failed");
        }
#else
        if (allowGraceful) {
            // First try to kill the process nicely and give it a 10 second
            // grace period before bringing out the big guns and terminate it
            // immediately
            if (kill(pid, SIGTERM) == -1) {
                if (errno == ESRCH) {
                    // Dead..
                    return;
                }
                throw std::system_error(
                        errno,
                        std::system_category(),
                        "ProcessMonitorImpl::terminate: kill(SIGTERM) failed");
            }

            // Give the process a 10 second grace window
            const auto timeout =
                    std::chrono::steady_clock::now() + std::chrono::seconds{10};
            do {
                if (!isRunning()) {
                    return;
                }
                std::this_thread::sleep_for(std::chrono::microseconds{100});
            } while (std::chrono::steady_clock::now() < timeout);
        }

        if (kill(pid, SIGKILL) == -1 && errno != ESRCH) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    "ProcessMonitorImpl::terminate: kill(SIGKILL) failed");
        }
#endif
    }

    void start() {
        thread = std::thread([this]() {
            if (name.empty()) {
                cb_set_thread_name("procmon");
            } else {
                cb_set_thread_name(name.c_str());
            }
            while (active) {
                auto locked = state.lock();

                // Wait for either the shutdown condvar to be notified, or for
                // 1s. If we hit the timeout then time to check the parent.
                if (shutdown_cv.wait_for(locked.as_lock(),
                                         std::chrono::seconds(1),
                                         [this] { return !active; })) {
                    // No longer monitoring - exit thread.
                    return;
                }

                // Check the monitored process and notify if it is gone (then
                // stop the monitor thread as the process won't come alive
                // once dead ;))
                if (isRunning(locked)) {
                    // Check if the process timed out if the user wanted
                    // set a timeout (used by cluster_testapp to kill the
                    // memcached process earlier and print the logs if it
                    // timed out)
                    if (locked->timeoutHandler) {
                        if (std::chrono::steady_clock::now() >
                            locked->timeout) {
                            locked->timeoutHandler();
                            locked->timeoutHandler = {};
                        }
                    }
                } else {
                    active = false;
                }
            }
        });
    }

protected:
#ifdef WIN32
    ProcessMonitorImpl(HANDLE handle,
                       ProcessMonitor::TerminateHandler terminateHandler,
                       std::string name)
        : name(std::move(name)),
          handle(handle),
          terminateHandler(std::move(terminateHandler)) {
        if (handle == INVALID_HANDLE_VALUE) {
            throw std::logic_error("ProcessMonitorImpl: Invalid handle value");
        }
        description["pid"] = GetProcessId(handle);
    }
#else
    ProcessMonitorImpl(uint64_t pid,
                       ProcessMonitor::TerminateHandler terminateHandler,
                       std::string name)
        : name(std::move(name)),
          pid(pid_t(pid)),
          terminateHandler(std::move(terminateHandler)) {
        description["pid"] = pid;
    }
#endif

    const std::string name;
#ifdef WIN32
    HANDLE handle;
#else
    pid_t pid;
#endif

    std::thread thread;
    const ProcessMonitor::TerminateHandler terminateHandler;
    std::atomic_bool active{true};
    nlohmann::json description;
    struct State {
        std::unique_ptr<ExitCodeImpl> processExitCode;
        std::chrono::steady_clock::time_point timeout;
        ProcessMonitor::TimeoutHandler timeoutHandler;
    };
    folly::Synchronized<State, std::mutex> state;
    std::condition_variable shutdown_cv;

    virtual bool isRunning(
            folly::Synchronized<State, std::mutex>::LockedPtr& locked) = 0;
};

/// Class responsible for creating and monitor a child process
class ChildProcessMonitor : public ProcessMonitorImpl {
public:
    ChildProcessMonitor(const std::vector<std::string>& argv,
                        ProcessMonitor::TerminateHandler terminateHandler,
                        std::string name)
        : ProcessMonitorImpl(createProcess(argv),
                             std::move(terminateHandler),
                             std::move(name)) {
        for (auto& c : argv) {
            description["argv"].push_back(c);
        }
    };

#ifndef WIN32
    ~ChildProcessMonitor() override {
        int exitcode;
        waitpid(pid, &exitcode, 0);
    }
#endif

protected:
#ifdef WIN32
    static HANDLE createProcess(const std::vector<std::string>& argv) {
        std::string cmdline = "\"";
        cmdline.append(argv[0]);
        cmdline.append("\" ");
        for (std::size_t ii = 1; ii < argv.size(); ++ii) {
            cmdline.append(argv[ii]);
            cmdline.append(" ");
        }
        cmdline.pop_back();

        STARTUPINFO sinfo;
        PROCESS_INFORMATION pinfo;
        memset(&sinfo, 0, sizeof(sinfo));
        memset(&pinfo, 0, sizeof(pinfo));
        sinfo.cb = sizeof(sinfo);

        if (!CreateProcess(nullptr,
                           const_cast<char*>(cmdline.c_str()),
                           nullptr,
                           nullptr,
                           FALSE,
                           0,
                           nullptr,
                           nullptr,
                           &sinfo,
                           &pinfo)) {
            throw std::system_error(GetLastError(),
                                    std::system_category(),
                                    "ChildProcessMonitor::createProcess: "
                                    "CreateProcess() failed");
        }
        return pinfo.hProcess;
    }
#else
    static uint64_t createProcess(const std::vector<std::string>& command) {
        const auto pid = fork();
        if (pid == -1) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    "ChildProcessMonitor::createProcess: fork() failed");
        }

        if (pid != 0) {
            return pid;
        }

        // Child process
        std::vector<char*> argv;
        for (auto& c : command) {
            argv.push_back(const_cast<char*>(c.data()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], const_cast<char**>(argv.data()));
        // execvp only returns on error and set errrno
        throw std::system_error(
                errno,
                std::system_category(),
                "ChildProcessMonitor::createProcess: execvp() failed");
    }
#endif

    bool isRunning(folly::Synchronized<State, std::mutex>::LockedPtr& locked)
            override {
        if (locked->processExitCode) {
            // We've already determined that it is dead
            return false;
        }

#ifdef WIN32
        DWORD exitcode = 0;
        if (!GetExitCodeProcess(handle, &exitcode)) {
            throw std::system_error(GetLastError(),
                                    std::system_category(),
                                    "ChildProcessMonitor::isRunning(): "
                                    "GetExitCodeProcess() failed");
        }
        if (exitcode == STILL_ACTIVE) {
            return true;
        }
        locked->processExitCode =
                std::make_unique<ExitCodeImpl>(uint32_t(exitcode));
#else
        // In order to figure out if the child died we need to try to
        // grab its exit status (kill will succeed as long as the process
        // is zombie)
        int exitcode;
        pid_t ret;
        do {
            ret = waitpid(pid, &exitcode, WNOHANG);
        } while (ret == reinterpret_cast<pid_t>(-1) && errno == EINTR);

        if (ret == 0) {
            // Process still running!
            return true;
        }

        if (ret == pid) {
            locked->processExitCode =
                    std::make_unique<ExitCodeImpl>(uint32_t(exitcode));
        } else {
            throw std::system_error(errno,
                                    std::system_category(),
                                    "ChildProcessMonitor::isRunning(): Unknown "
                                    "error from waitpid");
        }
#endif
        terminateHandler(*locked->processExitCode);
        return false;
    }
};

/// Class responsible for monitoring a process which isn't a child
/// of the current process (needs its own way to determine if it is
/// running
class OtherProcessMonitor : public ProcessMonitorImpl {
public:
#ifdef WIN32
    OtherProcessMonitor(uint64_t pid,
                        ProcessMonitor::TerminateHandler terminateHandler,
                        std::string name)
        : ProcessMonitorImpl(
                  OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid)),
                  std::move(terminateHandler),
                  std::move(name)) {
    }
#else
    OtherProcessMonitor(uint64_t pid,
                        ProcessMonitor::TerminateHandler terminateHandler,
                        std::string name)
        : ProcessMonitorImpl(
                  pid, std::move(terminateHandler), std::move(name)) {
    }
#endif

protected:
    bool isRunning(folly::Synchronized<State, std::mutex>::LockedPtr& locked)
            override {
        if (locked->processExitCode) {
            // We've already determined that it is dead
            return false;
        }

#ifdef WIN32
        if (WaitForSingleObject(handle, 0) == WAIT_TIMEOUT) {
            return true;
        }
        locked->processExitCode = std::make_unique<ExitCodeImpl>();
#else
        int ret;
        do {
            ret = kill(pid, 0);
        } while (ret == -1 && errno == EINTR);

        if (ret == 0) {
            return true;
        }
        if (errno != ESRCH) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    "OtherProcessMonitor::isRunning(): kill failed");
        }

        locked->processExitCode = std::make_unique<ExitCodeImpl>();
#endif
        terminateHandler(*locked->processExitCode);
        return false;
    }
};

/// ProcessMonitorProxy is a class used to make sure that we can shut down
/// the thread monitoring the underlying process before trying to delete
/// the owning object which would potentially cause a virtual function
/// to be called from the thread while the class was in the destructor.
class ProcessMonitorProxy : public ProcessMonitor {
public:
    ~ProcessMonitorProxy() override {
        if (!monitor) {
            return;
        }
        monitor->terminateMonitorThread();
    }

    bool isRunning() override {
        if (!monitor) {
            return false;
        }
        return monitor->isRunning();
    }
    ExitCode& getExitCode() override {
        if (!monitor) {
            throw std::logic_error(
                    "ProcessMonitorProxy::getExitCode(): monitor not created");
        }
        return monitor->getExitCode();
    }
    nlohmann::json describe() const override {
        if (!monitor) {
            throw std::logic_error(
                    "ProcessMonitorProxy::describe(): monitor not created");
        }
        return monitor->describe();
    }

    void terminate(bool allowGraceful) override {
        if (!monitor) {
            throw std::logic_error(
                    "ProcessMonitorProxy::terminate(): monitor not created");
        }

        monitor->terminate(allowGraceful);
    }

    void setTimeoutHander(std::chrono::seconds timeout,
                          TimeoutHandler handler) override {
        if (!monitor) {
            throw std::logic_error(
                    "ProcessMonitorProxy::setTimeoutHander(): monitor not "
                    "created");
        }

        monitor->setTimeoutHander(timeout, std::move(handler));
    }

    ProcessMonitorProxy(const std::vector<std::string>& argv,
                        TerminateHandler terminateHandler,
                        std::string threadname)
        : monitor(std::make_unique<ChildProcessMonitor>(
                  argv, terminateHandler, std::move(threadname))) {
        monitor->start();
    }

    ProcessMonitorProxy(uint64_t pid,
                        TerminateHandler terminateHandler,
                        std::string name)
        : monitor(std::make_unique<OtherProcessMonitor>(
                  pid, terminateHandler, std::move(name))) {
        monitor->start();
    }

protected:
    std::unique_ptr<ProcessMonitorImpl> monitor;
};

std::unique_ptr<ProcessMonitor> ProcessMonitor::create(
        const std::vector<std::string>& argv,
        TerminateHandler terminateHandler,
        std::string name) {
    return std::make_unique<ProcessMonitorProxy>(
            argv, terminateHandler, std::move(name));
}

std::unique_ptr<ProcessMonitor> ProcessMonitor::create(
        uint64_t pid, TerminateHandler terminateHandler, std::string name) {
    return std::make_unique<ProcessMonitorProxy>(
            pid, terminateHandler, std::move(name));
}

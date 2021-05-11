/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <nlohmann/json_fwd.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * The ProcessMonitor class allows for monitor of another process (and get
 * a callback when it terminates).
 *
 * It is implemented by using a dedicated thread which wakes up every second
 * to see if the monitored process is still alive, so unfortunately the
 * monitored process may have been dead up to a second a second until
 * the notification arrives).
 */
class ProcessMonitor {
public:
    class ExitCode {
    public:
        virtual ~ExitCode() = default;
        virtual std::string to_string() const = 0;
        virtual nlohmann::json to_json() const = 0;
        virtual bool isSuccess() const = 0;
    };

    using TerminateHandler = std::function<void(const ExitCode&)>;
    using TimeoutHandler = std::function<void()>;

    virtual ~ProcessMonitor() = default;

    /// Is the monitored process still running?
    virtual bool isRunning() = 0;

    /// Get the result code from the process (throws if still running)
    virtual ExitCode& getExitCode() = 0;

    /// Get a description of the process (intended use is for unit tests)
    virtual nlohmann::json describe() const = 0;

    /**
     * Terminate the child process immediately (The installed TerminateHandler
     * will be called)
     *
     * @param allowGraceful If set to true allow a graceful shutdown on
     *                      platforms which supports that
     *
     * @throws std::system_error upon failure
     */
    virtual void terminate(bool allowGraceful = true) = 0;

    /**
     * Set a timeout handler which will be called if the process hasn't
     * terminated within the provided time. By default there is no timeout
     *
     * @param timeout The number of seconds the process may run
     * @param handler The handler to call _once_ when the timer expire
     */
    virtual void setTimeoutHander(std::chrono::seconds timeout,
                                  TimeoutHandler handler) = 0;

    /**
     * Create a ProcessMonitor to watch the provided command
     *
     * @param argv The argument vector to start (use a std::string instead
     *             of std::string_view to ease the use when using return
     *             values from functions which returns stack objects)
     * @param terminateHandler The TerminateHandler to call when the process
     *                         terminates
     * @param threadname For platforms which support thread naming this
     *                   would be the name of the tread used to monitor
     *                   the life of the child process.
     * @return The newly created ProcessMonitor
     * @throws std::system_error for system failures
     */
    static std::unique_ptr<ProcessMonitor> create(
            const std::vector<std::string>& argv,
            TerminateHandler terminateHandler,
            std::string threadname = {});

    /**
     * Create a ProcessMonitor to watch the provided pid. The terminate
     * handler may not be able to provide the correct exit information
     * (for instance on Unix when specifying a pid which isn't a child
     * process). In such cases uint32_t(-1) is passed for the status
     * value
     *
     * @param pid The pid to monitor
     * @param terminateHandler The TerminateHandler to call when the process
     *                         terminates
     * @param threadname For platforms which support thread naming this
     *                   would be the name of the tread used to monitor
     *                   the life of the other process.
     * @return The newly created ProcessMonitor
     * @throws std::system_error for system failures
     */
    static std::unique_ptr<ProcessMonitor> create(
            uint64_t pid,
            TerminateHandler terminateHandler,
            std::string name = {});
};

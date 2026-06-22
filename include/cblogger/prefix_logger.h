/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <cblogger/logger.h>

namespace cb::logger {

/**
 * A logger that prefixes all log messages with a context.
 */
class PrefixLogger : public Logger {
public:
    PrefixLogger(const std::string& name, std::shared_ptr<Logger> baseLogger);

    // Keep the spdlog::level::level_enum compatibility overload of
    // logWithContext (defined on Logger) visible; overriding the Level-typed
    // virtual below would otherwise hide it.
    using Logger::logWithContext;

    void logWithContext(Level lvl, std::string_view msg, Json ctx) override;

    /**
     * Set the prefix for the logger.
     *
     * @param context The prefix to be added to a JSON context message
     * @param fmt The prefix to be added to the formatted message
     */
    void setPrefix(nlohmann::ordered_json context, std::string_view fmt);

    const nlohmann::ordered_json& getContextPrefix() const {
        return contextPrefix;
    }

    const std::string& getFmtPrefix() const {
        return fmtPrefix;
    }

protected:
    /**
     * Append the source object to the destination context.
     */
    void mergeContext(Json& dest, Json src) const;

private:
    /**
     * The prefix to be added to a JSON context message.
     */
    nlohmann::ordered_json contextPrefix;

    /**
     * The prefix to be added to a fmtlib formatted message.
     * Note: this is currently not used for formatting, but can be read back
     * from the instance, which is currently used.
     */
    std::string fmtPrefix;
};

} // namespace cb::logger

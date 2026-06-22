/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cblogger/prefix_logger.h>

#include <algorithm>

cb::logger::PrefixLogger::PrefixLogger(const std::string& name,
                                       std::shared_ptr<Logger> baseLogger)
    : Logger(name, baseLogger->getSpdLogger()),
      contextPrefix(nlohmann::ordered_json::object()) {
}

void cb::logger::PrefixLogger::setPrefix(nlohmann::ordered_json context,
                                         std::string_view fmt) {
    contextPrefix = std::move(context);
    fmtPrefix = fmt;
}

void cb::logger::PrefixLogger::logWithContext(Level lvl,
                                              std::string_view msg,
                                              Json ctx) {
    auto finalCtx = Json(contextPrefix);
    mergeContext(finalCtx, std::move(ctx));
    Logger::logWithContext(lvl, msg, std::move(finalCtx));
}

void cb::logger::PrefixLogger::mergeContext(Json& dest, Json src) const {
    fixupContext(dest);
    fixupContext(src);
    auto& destObj = dest.get_ref<Json::object_t&>();
    auto& srcObj = src.get_ref<Json::object_t&>();
    std::ranges::move(srcObj, std::back_inserter(destObj));
}

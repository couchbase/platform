/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include "common.h"

#include <nlohmann/json_fwd.hpp>
#include <filesystem>
#include <functional>
#include <unordered_set>

namespace cb::crypto {

/**
 * Find all the DEKs in use in the specified directory
 *
 * @param directory the directory to search
 * @param filefilter a function to filter out files we're not interested in
 *                   (return true to inspect the file, false to skip it)
 * @param error a function to call if we encounter an error
 */
std::unordered_set<std::string> findDeksInUse(
        const std::filesystem::path& directory,
        const std::function<bool(const std::filesystem::path&)>& filefilter,
        const std::function<void(std::string_view, const nlohmann::json&)>&
                error);

/**
 * Iterate over all files in the specified directory and potentially
 * rewrite all files.
 *
 * @param directory The directory to scan
 * @param filefilter a function to filter out files we're not interested in
 *                   (return true to inspect the file, false to skip it)
 * @param derivation_key The key to use when rewriting the files (if empty the
 *                       file will be written unencrypted)
 * @param key_lookup_function A function used to look up encryption keys from
 *            the id
 * @param error a callback to add log messages when errors occurs
 * @param unencrypted_extension The extension to use for unencrypted files
 */
void maybeRewriteFiles(
        const std::filesystem::path& directory,
        const std::function<bool(const std::filesystem::path&,
                                 std::string_view)>& filefilter,
        SharedKeyDerivationKey derivation_key,
        const std::function<SharedKeyDerivationKey(std::string_view)>&
                key_lookup_function,
        const std::function<void(std::string_view, const nlohmann::json&)>&
                error,
        std::string_view unencrypted_extension = ".txt");

} // namespace cb::crypto

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

#include <nlohmann/json.hpp>
#include <filesystem>

namespace cb::crypto {

namespace dump_keys {

class DumpKeysError : public std::runtime_error {
public:
    explicit DumpKeysError(const std::string& msg);
};

/// Exception thrown when dump-keys returns a non-zero exit code
class ExecuteError : public DumpKeysError {
public:
    ExecuteError(int ec, std::string out, std::string err);
    int ec;
    const std::string out;
    const std::string err;
};

class IncorrectPasswordError : public DumpKeysError {
public:
    IncorrectPasswordError();
};

class InvalidOutputError : public DumpKeysError {
public:
    InvalidOutputError(std::string msg, std::string out);
    const std::string msg;
    const std::string out;
};

class InvalidFormatError : public DumpKeysError {
public:
    InvalidFormatError(std::string msg, nlohmann::json json);
    const std::string msg;
    const nlohmann::json json;
};

class KeyLookupError : public DumpKeysError {
public:
    KeyLookupError(std::string id, std::string error);
    const std::string id;
    const std::string error;
};

class UnsupportedCipherError : public DumpKeysError {
public:
    UnsupportedCipherError(std::string id, std::string cipher);
    const std::string id;
    const std::string cipher;
};

} // namespace dump_keys

class DumpKeysRunner {
public:
    virtual ~DumpKeysRunner() = default;
    static std::unique_ptr<DumpKeysRunner> create(
            std::string password,
            std::filesystem::path executable,
            std::filesystem::path gosecrets_cfg);

    [[nodiscard]] virtual SharedKeyDerivationKey lookup(
            std::string_view id) const = 0;
};

} // namespace cb::crypto

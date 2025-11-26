/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <cbcrypto/dump_keys_runner.h>
#include <fmt/format.h>
#include <platform/base64.h>
#include <platform/dirutils.h>
#include <iostream>

namespace cb::crypto {

namespace dump_keys {
DumpKeysError::DumpKeysError(const std::string& msg) : std::runtime_error(msg) {
}

ExecuteError::ExecuteError(int ec, std::string out, std::string err)
    : DumpKeysError(
              fmt::format("Exit code: {} Stdout: {} Stderr: {}", ec, out, err)),
      ec(ec),
      out(std::move(out)),
      err(std::move(err)) {
}

IncorrectPasswordError::IncorrectPasswordError()
    : DumpKeysError("Incorrect password") {
}

InvalidOutputError::InvalidOutputError(std::string msg, std::string out)
    : DumpKeysError(msg), msg(std::move(msg)), out(std::move(out)) {
}

InvalidFormatError::InvalidFormatError(std::string msg, nlohmann::json json)
    : DumpKeysError(msg), msg(std::move(msg)), json(std::move(json)) {
}

KeyLookupError::KeyLookupError(std::string id, std::string error)
    : DumpKeysError(fmt::format("Failed to lookup {} due to {}", id, error)),
      id(std::move(id)),
      error(std::move(error)) {
}

UnsupportedCipherError::UnsupportedCipherError(std::string id,
                                               std::string cipher)
    : DumpKeysError(fmt::format(
              "Unsupported cipher ({}) specified for {}", cipher, id)),
      id(std::move(id)),
      cipher(std::move(cipher)) {
}
} // namespace dump_keys

static std::filesystem::path lookupExecutable(
        const std::filesystem::path& path,
        const std::filesystem::path& executable) {
    std::filesystem::path file;
    if (path.empty()) {
        file = "." / executable;
    } else {
        file = path / executable;
    }

#ifdef WIN32
    // On Windows, we need to check for the .exe extension
    file.replace_extension(".exe");
#endif

    if (!exists(file)) {
        throw dump_keys::DumpKeysError(
                fmt::format("The executable {} does not exist", file.string()));
    }
    return file;
}

class [[nodiscard]] DumpKeysRunnerImpl final : public DumpKeysRunner {
public:
    DumpKeysRunnerImpl(std::string password,
                       std::filesystem::path executable,
                       std::filesystem::path gosecrets_cfg)
        : password(std::move(password)),
          executable(std::move(executable)),
          gosecrets_cfg(std::move(gosecrets_cfg)) {
    }

    [[nodiscard]] SharedKeyDerivationKey lookup(
            std::string_view id) const override;

protected:
    static SharedKeyDerivationKey decodeJsonResponse(std::string_view id,
                                                     nlohmann::json json);

    const std::string password;
    const std::filesystem::path executable;
    const std::filesystem::path gosecrets_cfg;
};

SharedKeyDerivationKey DumpKeysRunnerImpl::lookup(
        const std::string_view id) const {
    boost::asio::io_service ios;
    std::future<std::string> out;
    std::future<std::string> err;
    boost::process::opstream in;

    const auto exec =
            lookupExecutable(executable.parent_path(), executable.filename());
    const auto gosecrets =
            lookupExecutable(executable.parent_path(), "gosecrets");

    std::vector<std::string> arguments = {{"--gosecrets",
                                           gosecrets.string(),
                                           "--config",
                                           gosecrets_cfg.string(),
                                           "--key-ids",
                                           std::string(id)}};

    if (!password.empty()) {
        arguments.emplace_back("--stdin-password");
    }

    if (getenv("CB_DUMP_KEYS_DEBUG")) {
        std::cout << exec.string();
        for (const auto& a : arguments) {
            std::cout << " [" << a << "]";
        }
        std::cout << std::endl;
    }

    boost::process::child c(
            exec.string(),
            boost::process::args(arguments),
            boost::process::std_in<in, boost::process::std_out> out,
            boost::process::std_err > err,
            ios);

    if (!password.empty()) {
        in << password << std::endl;
    }
    in.pipe().close();

    ios.run();

    c.wait();
    if (c.exit_code() != EXIT_SUCCESS) {
        if (c.exit_code() == 2) {
            throw dump_keys::IncorrectPasswordError();
        }

        throw dump_keys::ExecuteError(c.exit_code(), out.get(), err.get());
    }

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(out.get());
    } catch (std::exception& e) {
        throw dump_keys::InvalidOutputError(
                fmt::format("Failed to parse JSON output from dump-keys: {}",
                            e.what()),
                out.get());
    }

    return decodeJsonResponse(id, json);
}

SharedKeyDerivationKey DumpKeysRunnerImpl::decodeJsonResponse(
        const std::string_view id, nlohmann::json json) {
    if (!json.contains(id) || !json[id].is_object()) {
        throw dump_keys::InvalidFormatError(
                fmt::format("{} is not a key in the provided JSON (or not an "
                            "object)",
                            id),
                json);
    }

    const auto& object = json[id];
    if (!object.contains("result") || !object["result"].is_string()) {
        throw dump_keys::InvalidFormatError(
                fmt::format("{} is not a key in the provided JSON (or not a "
                            "string)",
                            id),
                object);
    }

    if (!object.contains("result") || !object["result"].is_string()) {
        throw dump_keys::InvalidFormatError(
                fmt::format("{} is not a key in the provided JSON (or not a "
                            "string)",
                            id),
                object);
    }

    if (!object.contains("response")) {
        throw dump_keys::InvalidFormatError(
                "'response' is not a key in the provided JSON", object);
    }

    const auto result = object["result"].get<std::string>();
    if (result == "error") {
        throw dump_keys::KeyLookupError(std::string{id},
                                        object["response"].dump());
    }

    // @todo MB-63550 requested this to be "success"
    if (result != "raw-aes-gcm" && result != "success") {
        throw dump_keys::KeyLookupError(
                std::string{id},
                fmt::format("Invalid value for result: {}", result));
    }

    auto& response = object["response"];
    if (!response.is_object()) {
        throw dump_keys::InvalidFormatError("'response' is not an object",
                                            object);
    }
    if (!response.contains("key") || !response["key"].is_string()) {
        throw dump_keys::InvalidFormatError(
                "'key' is not a key in the provided JSON", object["response"]);
    }
    if (response.contains("cipher") &&
        response["cipher"].get<std::string>() != "AES_128_GCM") {
        throw dump_keys::UnsupportedCipherError(
                std::string{id}, response["cipher"].get<std::string>());
    }
    return std::make_unique<KeyDerivationKey>(
            std::string{id},
            Cipher::AES_256_GCM,
            base64::decode(response["key"].get<std::string>()));
}

std::unique_ptr<DumpKeysRunner> DumpKeysRunner::create(
        std::string password,
        std::filesystem::path executable,
        std::filesystem::path gosecrets) {
    return std::make_unique<DumpKeysRunnerImpl>(
            std::move(password), std::move(executable), std::move(gosecrets));
}

} // namespace cb::crypto

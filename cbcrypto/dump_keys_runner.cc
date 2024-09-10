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

class [[nodiscard]] DumpKeysRunnerImpl final : public DumpKeysRunner {
public:
    DumpKeysRunnerImpl(std::string password,
                       std::filesystem::path executable,
                       std::filesystem::path gosecrets)
        : password(std::move(password)),
          executable(std::move(executable)),
          gosecrets(std::move(gosecrets)) {
    }

    [[nodiscard]] SharedEncryptionKey lookup(
            std::string_view id) const override;

protected:
    static SharedEncryptionKey decodeJsonResponse(std::string_view id,
                                                  nlohmann::json json);

    /// MB-63548 requests dump-keys to do this internally
    /// MB-63598 requests dump-keys to also be able to look up bucket keys
    ///          (instead of having to use dump-bucket-deks)
    [[nodiscard]] std::pair<std::string, std::filesystem::path> lookupKeyKind(
            std::string_view id) const;

    const std::string password;
    const std::filesystem::path executable;
    const std::filesystem::path gosecrets;
};

SharedEncryptionKey DumpKeysRunnerImpl::lookup(
        const std::string_view id) const {
    boost::asio::io_service ios;
    std::future<std::string> out;
    std::future<std::string> err;
    boost::process::opstream in;

    const auto [kind, path] = lookupKeyKind(id);

    std::vector<std::string> arguments = {
            {"--config", gosecrets.string(), "--key-ids", std::string(id)}};

    auto exec = executable;

    if (kind == "bucketDek") {
        arguments.emplace_back("--bucket");
        arguments.emplace_back(
                path.parent_path().parent_path().filename().string());
        exec = executable.parent_path() / "dump-bucket-deks";
    } else {
        arguments.emplace_back("--key-kind");
        arguments.emplace_back(kind);
    }

    if (!password.empty()) {
        arguments.emplace_back("--stdin-password");
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

SharedEncryptionKey DumpKeysRunnerImpl::decodeJsonResponse(
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
    return std::make_unique<DataEncryptionKey>(
            std::string{id},
            Cipher::AES_256_GCM,
            base64::decode(response["key"].get<std::string>()));
}

std::pair<std::string, std::filesystem::path> DumpKeysRunnerImpl::lookupKeyKind(
        std::string_view id) const {
    nlohmann::json configuration;
    try {
        configuration = nlohmann::json::parse(cb::io::loadFile(gosecrets));
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("Failed to read and parse {}: {}",
                                             gosecrets.string(),
                                             e.what()));
    }
    for (const auto& entry : configuration["storedKeys"]) {
        std::error_code ec;
        for (auto const& dir_entry :
             std::filesystem::recursive_directory_iterator{
                     entry["path"].get<std::string>(), ec}) {
            if (dir_entry.is_regular_file() &&
                dir_entry.path().filename() == id) {
                return {entry["kind"].get<std::string>(), dir_entry.path()};
            }
        }
    }

    throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            fmt::format("Failed to detect the key kind for {} from the "
                        "configuration data in {}",
                        id,
                        gosecrets.string()));
}

std::unique_ptr<DumpKeysRunner> DumpKeysRunner::create(
        std::string password,
        std::filesystem::path executable,
        std::filesystem::path gosecrets) {
    return std::make_unique<DumpKeysRunnerImpl>(
            std::move(password), std::move(executable), std::move(gosecrets));
}

} // namespace cb::crypto

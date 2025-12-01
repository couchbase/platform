/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cbcrypto/dump_keys_runner.h>
#include <cbcrypto/file_reader.h>
#include <cbcrypto/key_store.h>
#include <fmt/format.h>
#include <platform/command_line_options_parser.h>
#include <platform/getpass.h>
#include <filesystem>
#include <iostream>

#ifdef WIN32
#define INSTALL_ROOT "C:/Program Files/Couchbase/Server"
#else
#define INSTALL_ROOT DESTINATION_ROOT
#endif

using namespace cb::crypto;

/// Exit code for incorrect password returned from dump-deks
static constexpr int EXIT_INCORRECT_PASSWORD = 2;

static std::string password;
static std::unique_ptr<DumpKeysRunner> dump_keys_runner;
static KeyStore keyStore;

/// The key lookup callback gets called from the FileReader whenever it
/// encounters an encrypted file. It'll keep the keys around in a key store
/// to avoid running dump-keys again in the case where one is trying to
/// dump multiple files (in the case the same key is used for multiple
/// files)
static SharedKeyDerivationKey key_lookup_callback(std::string_view id) {
    if (id == KeyDerivationKey::PasswordKeyId) {
        return std::make_shared<KeyDerivationKey>(
                std::string{id},
                Cipher::AES_256_GCM,
                password,
                KeyDerivationMethod::PasswordBased);
    }

    auto ret = keyStore.lookup(id);
    if (ret) {
        return ret;
    }

    if (dump_keys_runner) {
        auto key = dump_keys_runner->lookup(id);
        if (key) {
            // add for later requests
            keyStore.add(key);
        }
        return key;
    }
    return {};
}

static void usage(cb::getopt::CommandLineOptionsParser& instance,
                  int exitcode) {
    std::cerr << R"(Usage: cbcat [options] file(s)

Options:

)" << instance << R"(

You may set the environment variable CB_DUMP_KEYS_DEBUG to enable
debug output to see the command line used to run the dump-keys
program.

)";
    std::exit(exitcode);
}

static void stdinOveruse() {
    std::cerr << "stdin may only be used once (password or key store)\n";
    std::exit(EXIT_FAILURE);
}

void populateKeyStore(std::string_view data) {
    try {
        const auto json = nlohmann::json::parse(data);
        if (json.is_array()) {
            for (const auto& entry : json) {
                auto key = std::make_shared<KeyDerivationKey>();
                *key = entry;
                keyStore.add(key);
            }
        } else if (json.is_object()) {
            auto key = std::make_shared<KeyDerivationKey>();
            *key = json;
            keyStore.add(key);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse key store: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void readKeyStoreFromStdin() {
    std::string input;

    while (true) {
        std::array<char, 4096> buffer;
        auto nr = fread(buffer.data(), 1, buffer.size(), stdin);
        if (nr > 0) {
            input.append(buffer.data(), nr);
        } else if (feof(stdin)) {
            populateKeyStore(input);
            return;
        }

        if (ferror(stdin)) {
            std::system_error error(
                    errno, std::system_category(), "Failed to read from stdin");
            std::cerr << error.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char** argv) {
    using cb::getopt::Argument;
    cb::getopt::CommandLineOptionsParser parser;

    std::string dumpKeysExecutable = INSTALL_ROOT "/bin/dump-keys";
    std::string gosecrets =
            INSTALL_ROOT "/var/lib/couchbase/config/gosecrets.cfg";
    bool printHeader = false;
    bool withKeyStore = false;
    bool stdinUsed = false;

    parser.addOption({
            [&dumpKeysExecutable](auto value) {
                dumpKeysExecutable = std::string{value};
            },
            "with-dump-keys",
            Argument::Required,
            "filename",
            fmt::format("The \"dump-keys\" binary to use (by default {}",
                        dumpKeysExecutable),
    });
    parser.addOption(
            {[&gosecrets](auto value) { gosecrets = std::string{value}; },
             "with-gosecrets",
             Argument::Required,
             "filename",
             fmt::format("The location of gosecrets.cfg (by default {})",
                         gosecrets)});
    parser.addOption(
            {[&stdinUsed](auto value) {
                 if (value == "-") {
                     if (stdinUsed) {
                         stdinOveruse();
                     }
                     password = cb::getpass();
                     stdinUsed = true;
                 } else {
                     password = std::string{value};
                 }
             },
             "password",
             Argument::Required,
             "password",
             "The password to use for authentication or as decryption key "
             "(use '-' to read from standard input)"});

    parser.addOption(
            {[&withKeyStore, &stdinUsed](auto value) {
                 withKeyStore = true;

                 if (value == "-") {
                     if (stdinUsed) {
                         stdinOveruse();
                     }
                     stdinUsed = true;
                     readKeyStoreFromStdin();
                 } else {
                     populateKeyStore(value);
                 }
             },
             "with-keystore",
             Argument::Required,
             "json or -",
             "The JSON containing the keystore to use (use '-' to read from "
             "standard input)"});

    parser.addOption({[&printHeader](auto value) { printHeader = true; },
                      "print-header",
                      "Print a header with the file name before the content of "
                      "the file"});
    parser.addOption({[](auto) {
                          std::cout << "Couchbase Server " << PRODUCT_VERSION
                                    << std::endl;
                          std::exit(EXIT_SUCCESS);
                      },
                      "version",
                      "Print program version and exit"});
    parser.addOption({[&parser](auto) { usage(parser, EXIT_SUCCESS); },
                      "help",
                      "This help text"});

    auto arguments = parser.parse(
            argc, argv, [&parser]() { usage(parser, EXIT_FAILURE); });

    if (!withKeyStore) {
        dump_keys_runner =
                DumpKeysRunner::create(password, dumpKeysExecutable, gosecrets);
    }

    for (const auto& file : arguments) {
        if (printHeader) {
            std::cout << std::endl
                      << file << std::endl
                      << std::string(file.length(), '=') << std::endl;
        }
        try {
            auto reader = FileReader::create(file, key_lookup_callback);
            reader->set_max_allowed_chunk_size(
                    std::numeric_limits<uint32_t>::max());
            std::vector<uint8_t> blob(8192);
            while (!reader->eof()) {
                auto nr = reader->read(blob);
                std::cout.write(reinterpret_cast<const char*>(blob.data()), nr);
                std::cout.flush();
            }
        } catch (const cb::crypto::dump_keys::IncorrectPasswordError& e) {
            std::cerr << e.what() << std::endl;
            std::exit(EXIT_INCORRECT_PASSWORD);

        } catch (const std::exception& e) {
            std::cerr << "Fatal error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

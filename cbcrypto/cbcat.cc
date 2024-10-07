/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "platform/base64.h"

#include <cbcrypto/dump_keys_runner.h>
#include <cbcrypto/file_reader.h>
#include <cbcrypto/key_store.h>
#include <fmt/format.h>
#include <platform/command_line_options_parser.h>
#include <platform/getpass.h>
#include <filesystem>
#include <iostream>

using namespace cb::crypto;

static std::unique_ptr<DumpKeysRunner> dump_keys_runner;

/// The key lookup callback gets called from the FileReader whenever it
/// encounters an encrypted file. It'll keep the keys around in a key store
/// to avoid running dump-keys again in the case where one is trying to
/// dump multiple files (in the case the same key is used for multiple
/// files)
static SharedEncryptionKey key_lookup_callback(std::string_view id) {
    static KeyStore keyStore;
    auto ret = keyStore.lookup(id);
    if (ret) {
        return ret;
    }

    auto key = dump_keys_runner->lookup(id);
    if (key) {
        // add for later requests
        keyStore.add(key);
    }
    return key;
}

static void usage(cb::getopt::CommandLineOptionsParser& instance,
                  int exitcode) {
    std::cerr << R"(Usage: cbcat [options] file(s)

Options:

)" << instance << std::endl
              << std::endl;
    std::exit(exitcode);
}

int main(int argc, char** argv) {
    using cb::getopt::Argument;
    cb::getopt::CommandLineOptionsParser parser;

    std::string dumpKeysExecutable = DESTINATION_ROOT "/bin/dump-keys";
    std::string gosecrets =
            DESTINATION_ROOT "/var/lib/couchbase/config/gosecrets.cfg";
    std::string password;
    bool printHeader = false;

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
            {[&password](auto value) {
                 if (value == "-") {
                     password = cb::getpass();
                 } else {
                     password = std::string{value};
                 }
             },
             "password",
             Argument::Required,
             "password",
             "The password to use for authentication (use '-' to read from "
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

    dump_keys_runner =
            DumpKeysRunner::create(password, dumpKeysExecutable, gosecrets);

    for (const auto& file : arguments) {
        if (printHeader) {
            std::cout << fmt::format("{}\n{}\n",
                                     file,
                                     std::string(file.length(), '='))
                      << std::endl;
        }
        try {
            auto reader = FileReader::create(file, key_lookup_callback);
            std::string message;
            while (!(message = reader->nextChunk()).empty()) {
                std::cout << message;
                std::cout.flush();
            }
        } catch (const std::exception& e) {
            std::cerr << "Fatal error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/command_line_options_parser.h>
#include <platform/dirutils.h>
#include <platform/terminal_color.h>
#include <deque>
#include <filesystem>
#include <iostream>

using namespace cb::getopt;
using cb::io::loadFile;
using cb::terminal::TerminalColor;

/**
 * Inspect the provided path to check if the file contains #pragma once
 * if the path represent a regular file with a .h extension
 *
 * @param path the filesystem path to check
 * @return true if the file is OK (the above is true), false otherwise
 */
static bool inspect_file(const std::filesystem::path& path) {
    std::error_code ec;
    if (!is_regular_file(path, ec) || // not a normal file
        !path.has_extension() || // no extension
        path.extension() != ".h") { // not a header file
        // Ignore the file
        return true;
    }

    try {
        const auto content = loadFile(path.generic_string());
        if (content.find("#pragma once") == std::string::npos) {
            std::cerr << TerminalColor::Red << "FAIL: \""
                      << path.generic_string() << TerminalColor::Reset
                      << "\" does not contain #pragma once" << std::endl;
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        std::cerr << TerminalColor::Red
                  << "FAIL: Exception occurrec while inspecting \""
                  << path.generic_string() << TerminalColor::Reset
                  << "\": " << exception.what() << std::endl;
    }

    return false;
}

int main(int argc, char** argv) {
    std::filesystem::path source_root = std::filesystem::current_path();
    std::deque<std::string> exclude;

    CommandLineOptionsParser parser;
    parser.addOption({[&source_root](auto value) { source_root = value; },
                      "rootdir",
                      Argument::Required,
                      "dir",
                      "Directory to check header files in, defaults to the "
                      "current working directory"});
    parser.addOption({[&exclude](auto value) { exclude.emplace_back(value); },
                      "exclude",
                      Argument::Required,
                      "file",
                      "File to exclude relative from rootdir"});
    parser.addOption({[&parser](auto) {
                          std::cerr << "check_pragma_once [options]"
                                    << std::endl;
                          parser.usage(std::cerr);
                          std::exit(EXIT_SUCCESS);
                      },
                      "help",
                      "Print this help"});

    const auto arguments = parser.parse(argc, argv, [&parser]() {
        std::cerr << std::endl;
        parser.usage(std::cerr);
        std::exit(EXIT_FAILURE);
    });

    if (!exists(source_root)) {
        std::cerr << TerminalColor::Red
                  << "Fatal: " << source_root.generic_string()
                  << TerminalColor::Reset << " does not exists";
        return EXIT_FAILURE;
    }

    std::deque<std::filesystem::path> ignore;
    for (const auto& f : exclude) {
        ignore.emplace_back(source_root / f);
    }

    int exitcode = EXIT_SUCCESS;
    std::error_code ec;
    for (const auto& itr :
         std::filesystem::recursive_directory_iterator(source_root, ec)) {
        if (std::find(ignore.begin(), ignore.end(), itr.path()) ==
            ignore.end()) {
            if (!inspect_file(itr.path())) {
                exitcode = EXIT_FAILURE;
            }
        }
    }

    return exitcode;
}

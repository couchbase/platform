/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace cb::getopt {

/// Enum used to specify the arguments allowed to an option
enum class Argument { No, Optional, Required };

/// Each command line option
struct Option {
    /// Create a long option without any arguments
    Option(std::function<void(std::string_view)> cb,
           std::string lo,
           std::string description)
        : callback(std::move(cb)),
          longOption(std::move(lo)),
          argument(Argument::No),
          description(std::move(description)) {
    }

    /// Create a long option which takes an argument
    Option(std::function<void(std::string_view)> cb,
           std::string lo,
           Argument argument,
           std::string name,
           std::string description)
        : callback(std::move(cb)),
          longOption(std::move(lo)),
          argument(argument),
          name(std::move(name)),
          description(std::move(description)) {
    }

    /// Create an option with an option with either a short name or a long
    /// name and no argument
    Option(std::function<void(std::string_view)> cb,
           std::optional<char> so,
           std::string lo,
           std::string description)
        : callback(std::move(cb)),
          shortOption(std::move(so)),
          longOption(std::move(lo)),
          argument(Argument::No),
          description(std::move(description)) {
    }

    /// Create an option with an option with either a short name or a long
    /// name with an argument
    Option(std::function<void(std::string_view)> cb,
           std::optional<char> so,
           std::string lo,
           Argument argument,
           std::string name,
           std::string description)
        : callback(std::move(cb)),
          shortOption(std::move(so)),
          longOption(std::move(lo)),
          argument(argument),
          name(std::move(name)),
          description(std::move(description)) {
    }

    /// The callback to call when the option found (with the argument)
    std::function<void(std::string_view)> callback;
    /// The short option (single character) for the option
    std::optional<char> shortOption;
    /// The long option to use
    std::string longOption;
    /// The arguments the option may take
    Argument argument;
    /// The value name in help
    std::string name;
    /// The description
    std::string description;
};

/**
 * The command line option parser provides a callback driven method built
 * on top of "getopt".
 *
 * To use it you should add all of the options you want to deal with
 * and finally call parse() which will built up whatever datastructures
 * getopt wants, and finally loop calling getopt to deal with all of
 * options provided.
 *
 * After its done optind points to the first argument
 */
class CommandLineOptionsParser {
public:
    /**
     * Add a command line option to the list of command line options
     * to accept
     */
    void addOption(Option option);

    /**
     * Parse the command line options and call the callbacks for all
     * options found. A vector of the arguments is returned unless
     * the error callback was called. Given that it is based upon
     * getopt it will not touch optind so the caller may use that
     * as an indication for the next unparsed argument.
     *
     * @param argc argument count
     * @param argv argument vector
     * @param error an error callback for unknown options
     * @returns a vector containing all of the arguments
     */
    std::vector<std::string_view> parse(int argc,
                                        char* const* argv,
                                        std::function<void()> error) const;

    /// Print the common command line options to the output stream
    void usage(std::ostream&) const;

protected:
    std::vector<Option> options;
};

std::ostream& operator<<(std::ostream& os,
                         const CommandLineOptionsParser& parser);
} // namespace cb::getopt
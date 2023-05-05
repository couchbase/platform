/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/command_line_options_parser.h>

#include <fmt/format.h>
#include <getopt.h>
#include <platform/terminal_color.h>
#include <platform/terminal_size.h>
#include <sstream>
#include <unordered_map>

namespace cb::getopt {
void CommandLineOptionsParser::addOption(Option option) {
    if (!option.shortOption.has_value() && option.longOption.empty()) {
        throw std::invalid_argument(
                "addOption(): An option must have a short or a long option");
    }

    if (option.description.empty()) {
        throw std::invalid_argument(
                "addOption(): An option must have a description");
    }

    // Verify that the option hasn't already been set
    for (const auto& o : options) {
        if (o.shortOption.has_value() && option.shortOption.has_value() &&
            o.shortOption == option.shortOption) {
            throw std::runtime_error(fmt::format(
                    "addOption(): short option '{}' already registered",
                    option.shortOption.value()));
        }
        if (!o.longOption.empty() && !option.longOption.empty() &&
            o.longOption == option.longOption) {
            throw std::runtime_error(fmt::format(
                    "addOption(): long option '{}' already registered",
                    option.longOption));
        }
    }

    options.emplace_back(std::move(option));
}

std::vector<std::string_view> CommandLineOptionsParser::parse(
        int argc, char* const* argv, std::function<void()> error) const {
    std::string short_options;
    // We need to pass along an integer returned from the long options.
    // start on 256 as that's outside the char range and won't be in
    // conflict with any values passed by a user
    int next_short_option = 256;

    std::unordered_map<int, std::function<void(std::string_view)>> callbacks;
    std::vector<option> opt;
    for (const auto& o : options) {
        int so;
        if (o.shortOption.has_value()) {
            short_options.push_back(o.shortOption.value());
            if (o.argument == Argument::Required) {
                short_options.push_back(':');
            }
            so = int(o.shortOption.value());
        } else {
            so = next_short_option++;
        }

        const char* lo = nullptr;
        if (!o.longOption.empty()) {
            lo = o.longOption.data();
        }

        auto getArgument = [](Argument argument) {
            switch (argument) {
            case Argument::No:
                return no_argument;
            case Argument::Optional:
                return optional_argument;
            case Argument::Required:
                return required_argument;
            }
            throw std::invalid_argument("getArgument(): Unknown argument");
        };

        callbacks[so] = o.callback;
        opt.emplace_back(option{lo, getArgument(o.argument), nullptr, so});
    }

    opt.emplace_back(option{nullptr, 0, nullptr, 0});

    int cmd;
    while ((cmd = ::getopt_long(
                    argc, argv, short_options.c_str(), opt.data(), nullptr)) !=
           EOF) {
        auto iter = callbacks.find(cmd);
        if (iter == callbacks.end()) {
            error();
            return {};
        } else {
            if (optarg) {
                iter->second(optarg);
            } else {
                iter->second({});
            }
        }
    }

    std::vector<std::string_view> ret;
    for (auto ii = optind; ii < argc; ++ii) {
        ret.emplace_back(argv[ii]);
    }

    return ret;
}

void CommandLineOptionsParser::usage(std::ostream& out) const {
    std::vector<std::string> keys;
    std::size_t widest = 0;
    for (const auto& o : options) {
        std::stringstream ss;
        if (o.shortOption.has_value()) {
            ss << "-" << o.shortOption.value();
            if (!o.longOption.empty()) {
                ss << " or ";
            }
        }
        if (!o.longOption.empty()) {
            ss << "--" << o.longOption;
            if (o.argument != Argument::No) {
                if (o.argument == Argument::Optional) {
                    ss << "[=";
                } else {
                    ss << " ";
                }

                if (o.name.empty()) {
                    ss << "value";
                } else {
                    ss << o.name;
                }
                if (o.argument == Argument::Optional) {
                    ss << "]";
                }
            }
        }
        keys.emplace_back(ss.str());
        if (keys.front().size() > widest) {
            widest = keys.front().size();
        }
    }

    size_t dw;
    // We use two space indentations, two space between columns and two
    // characters at the end of the line
    const std::size_t indentation = 6;
    try {
        auto [width, height] = cb::terminal::getTerminalSize();
        (void)height;

        if ((widest > width) || (width - widest - indentation < 20)) {
            // the terminal is so narrow... don't try to format
            width = std::numeric_limits<size_t>::max();
        }

        dw = width - widest - indentation;
    } catch (const std::exception&) {
        dw = std::numeric_limits<size_t>::max();
    }

    for (std::size_t index = 0; index < options.size(); ++index) {
        using cb::terminal::TerminalColor;
        auto format = fmt::format("  {{:<{}}}", widest + 2);
        out << TerminalColor::Yellow << fmt::format(format, keys[index])
            << TerminalColor::Green;

        std::string_view descr = options[index].description;

        while (true) {
            if (descr.size() < dw) {
                out << descr.data() << std::endl;
                break;
            }

            auto idx = descr.rfind(' ', std::min(dw, descr.size()));
            if (idx == std::string::npos) {
                out << descr.data() << std::endl;
                break;
            }
            out.write(descr.data(), idx);
            out << std::endl;
            descr.remove_prefix(idx + 1);
            out << fmt::format(format, " ");
        }
        out << TerminalColor::Reset;
    }
}

std::ostream& operator<<(std::ostream& os,
                         const CommandLineOptionsParser& parser) {
    parser.usage(os);
    return os;
}
} // namespace cb::getopt

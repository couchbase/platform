/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <getopt.h>
#include <gsl/gsl-lite.hpp>
#include <platform/command_line_options_parser.h>
#include <platform/terminal_color.h>

using namespace cb::getopt;

/// Detect that an option must have at least a short or long option
TEST(CommandLineOptionsParserTest, NeedShortOrLongOption) {
    CommandLineOptionsParser parser;
    try {
        parser.addOption({[](auto value) {}, {}, {}, "Dummy option"});
        FAIL() << "It should not be possible to define options without a short "
                  "and long option";
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(
                "addOption(): An option must have a short or a long option",
                e.what());
    }
}

TEST(CommandLineOptionsParserTest, NeedDescription) {
    CommandLineOptionsParser parser;
    try {
        parser.addOption({[](auto value) {}, 'a', {}, {}});
        FAIL() << "It should not be possible to define options without a "
                  "description";
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ("addOption(): An option must have a description",
                     e.what());
    }
}

/// It should not be possible to define multiple options with the same
/// short or long option
TEST(CommandLineOptionsParserTest, DetectOptionAlreadyDefined) {
    CommandLineOptionsParser parser;
    Option option{[](auto value) {}, 'a', "option", "Dummy option"};
    parser.addOption(option);
    // change the long option but keep the same short option (should fail)
    option.longOption = "foo";
    try {
        parser.addOption(option);
        FAIL() << "Short option a should already be in use";
    } catch (const std::exception& msg) {
        EXPECT_STREQ("addOption(): short option 'a' already registered",
                     msg.what());
    }

    // change the long option back and change the short option
    option.longOption = "option";
    option.shortOption = 'b';
    try {
        parser.addOption(option);
        FAIL() << "Long option a should already be in use";
    } catch (const std::exception& msg) {
        EXPECT_STREQ("addOption(): long option 'option' already registered",
                     msg.what());
    }
}

TEST(CommandLineOptionsParserTest, TestParse) {
    std::vector<const char*> options{"argv0", "--first=firstarg"};
    CommandLineOptionsParser parser;

    bool found = false;
    parser.addOption({[&found](auto value) {
                          found = true;
                          EXPECT_EQ("firstarg", value);
                      },
                      {},
                      "first",
                      Argument::Optional,
                      "value",
                      "the first argument"});

    parser.addOption({[&found](auto value) {
                          found = true;
                          EXPECT_EQ("secondarg", value);
                      },
                      {},
                      "second",
                      Argument::Required,
                      "value",
                      "the first argument"});
    ::optind = 1;
    auto arguments = parser.parse(gsl::narrow_cast<int>(options.size()),
                                  const_cast<char* const*>(options.data()),
                                  []() { FAIL() << "An error occurred"; });
    EXPECT_TRUE(found);
    EXPECT_EQ(options.size(), optind);
    EXPECT_TRUE(arguments.empty());

    found = false;
    options = {"argv0", "--second", "secondarg", "third"};
    ::optind = 1;
    arguments = parser.parse(gsl::narrow_cast<int>(options.size()),
                             const_cast<char* const*>(options.data()),
                             []() { FAIL() << "An error occurred"; });
    EXPECT_TRUE(found);
    EXPECT_EQ(options.size() - 1, ::optind);
    EXPECT_EQ(1, arguments.size());
    EXPECT_EQ(std::string_view(options.back()), arguments.front());
}

TEST(CommandLineOptionsParserTest, TestParseError) {
    std::vector<const char*> options{
            "argv0", "--first=firstarg", "--unknown", "foo"};
    CommandLineOptionsParser parser;
    bool found = false;
    parser.addOption({[&found](auto value) {
                          found = true;
                          EXPECT_EQ("firstarg", value);
                      },
                      {},
                      "first",
                      Argument::Optional,
                      "value",
                      "the first argument"});

    bool error = false;
    ::optind = 1;
    auto arguments = parser.parse(gsl::narrow_cast<int>(options.size()),
                                  const_cast<char* const*>(options.data()),
                                  [&error]() { error = true; });
    ::optind = 1;
    EXPECT_TRUE(found);
    EXPECT_TRUE(error);
    EXPECT_TRUE(arguments.empty());
}

TEST(CommandLineOptionsParserTest, TestUsage) {
    const auto color = cb::terminal::isTerminalColorEnabled();
    cb::terminal::setTerminalColorSupport(false);
    CommandLineOptionsParser parser;

    parser.addOption({[&](auto value) {},
                      'h',
                      "host",
                      Argument::Optional,
                      "hostname",
                      "The host to connect to"});
    parser.addOption({[&](auto value) {}, "help", "This help page"});

    std::stringstream ss;
    ss << parser;
    EXPECT_EQ(
            "  -h or --host[=hostname]  The host to connect to\n  --help       "
            "            This help page\n",
            ss.str());

    if (color) {
        cb::terminal::setTerminalColorSupport(color);
    }
}

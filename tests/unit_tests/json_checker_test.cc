/*
 *     Copyright 2014-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <JSON_checker.h>
#include <folly/portability/GTest.h>
#include <platform/string_hex.h>
#include <array>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

class DeprecatedInterfaceValidatorTest : public ::testing::Test {
};

#define CHECK_JSON(X) checkUTF8JSON((const unsigned char *)X, sizeof(X) - 1)

TEST_F(DeprecatedInterfaceValidatorTest, SimpleJsonChecksOk) {
    EXPECT_TRUE(CHECK_JSON("{\"test\": 12}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, DeepJsonChecksOk) {
    EXPECT_TRUE(CHECK_JSON(
              "{\"test\": [[[[[[[[[[[[[[[[[[[[[[12]]]]]]]]]]]]]]]]]]]]]]}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BadDeepJsonIsNotOK) {
    EXPECT_FALSE(CHECK_JSON(
                 "{\"test\": [[[[[[[[[[[[[[[[[[[[[[12]]]]]]]]]]]]]]]]]]]]]]]]}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BadJsonStartingWithBraceIsNotOk) {
    EXPECT_FALSE(CHECK_JSON("{bad stuff}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BareValuesAreOk) {
    EXPECT_TRUE(CHECK_JSON("null"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BareNumbersAreOk) {
    EXPECT_TRUE(CHECK_JSON("99"));
}
TEST_F(DeprecatedInterfaceValidatorTest, BadUtf8IsNotOk) {
    EXPECT_FALSE(CHECK_JSON("{\"test\xFF\": 12}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', 0xff, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}
TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk2) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', 'a', 0xff, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}

TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk3) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', '1', '2', 0xfe, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}
TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk4) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', '1', '2', 0xfd, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}
TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk5) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'{', '"', 'k', '"', ':', '"', 0xfc, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}

class ValidatorTest : public ::testing::Test,
                      public ::testing::WithParamInterface<bool> {
public:
    void SetUp() override {
        validator = std::make_unique<JSON_checker::Validator>(GetParam());
    }

protected:
    std::unique_ptr<JSON_checker::Validator> validator;
};

TEST_P(ValidatorTest, SimpleValidatorTest) {
    const uint8_t value1[] = "{\"test\": 12}";
    EXPECT_TRUE(validator->validate(value1, sizeof(value1) - 1));
    EXPECT_FALSE(validator->validate(value1, sizeof(value1) - 3));
    EXPECT_TRUE(validator->validate(value1, sizeof(value1) - 1));
}

TEST_P(ValidatorTest, StringLengthTest) {
    for (int i = 0; i < 50; i++) {
        std::string json = R"({"test": ")" + std::string(i, 'x') + "\"}";
        EXPECT_TRUE(validator->validate(json)) << "Failed: " << json;
    }
}

TEST_P(ValidatorTest, ByteArrayValidatorTest) {
    const uint8_t value1[] = "{\"test\": 12}";
    std::vector<uint8_t> data;
    data.resize(sizeof(value1) - 1);
    memcpy(data.data(), value1, sizeof(value1) - 1);
    EXPECT_TRUE(validator->validate(data));
    data.resize(data.size() - 1);
    EXPECT_FALSE(validator->validate(data));
    data.emplace_back('}');
    EXPECT_TRUE(validator->validate(data));
}

TEST_P(ValidatorTest, StringValidatorTest) {
    std::string value("{\"test\": 12}");
    EXPECT_TRUE(validator->validate(value));
    value.append("}");
    EXPECT_FALSE(validator->validate(value));
    value.resize(value.length() - 1);
    EXPECT_TRUE(validator->validate(value));
}

TEST_P(ValidatorTest, NumberExponentValidatorTest) {
    EXPECT_TRUE(validator->validate("0e5"));
    EXPECT_TRUE(validator->validate("0E5"));
    EXPECT_TRUE(validator->validate("0.00e5"));
}

TEST_P(ValidatorTest, ControlCharactersTest) {
    // Code points up to 0x1f are all control characters which are illegal.
    for (char i = 0; i <= 0x1f; i++) {
        // Sufficiently large such that vectorisation kicks in
        std::string s = "\"Illegal code point>>";
        s.push_back(i);
        s += "<<inserted into a string\"";

        EXPECT_FALSE(validator->validate(s))
                << "Code point 0x" << std::hex << (int)i << std::dec;
    }
}

TEST_P(ValidatorTest, EquivalenceTest) {
    auto otherValidator =
            std::make_unique<JSON_checker::Validator>(!GetParam());

    for (int i = std::numeric_limits<char>::min();
         i <= std::numeric_limits<char>::max();
         i++) {
        // Sufficiently large such that vectorisation kicks in
        std::string s = "\"Test code>>";
        s.push_back((char)i);
        s += "<<in a string\"";

        EXPECT_EQ(validator->validate(s), otherValidator->validate(s))
                << "Validators do not agree when parsing 0x" << std::hex
                << (int)i << std::dec;
    }
}

/// Wrap the provided bytes in a JSON string sufficiently large such that
/// vectorisation kicks in
static std::string makeString(std::string_view bytes) {
    return "\"Test code>>" + std::string(bytes) + "<<in a string\"";
}

/// The UTF-8 encoding must be strictly validated (RFC 3629) by both
/// implementations
TEST_P(ValidatorTest, Utf8Test) {
    const std::vector<std::string> valid = {"\x7f", // U+007F
                                            "\xc2\x80", // U+0080
                                            "\xc3\xa9", // U+00E9
                                            "\xdf\xbf", // U+07FF
                                            "\xe0\xa0\x80", // U+0800
                                            "\xe2\x82\xac", // U+20AC
                                            "\xed\x9f\xbf", // U+D7FF
                                            "\xee\x80\x80", // U+E000
                                            "\xef\xbf\xbf", // U+FFFF
                                            "\xf0\x90\x80\x80", // U+10000
                                            "\xf0\x9f\x98\x80", // U+1F600
                                            "\xf4\x8f\xbf\xbf"}; // U+10FFFF
    for (const auto& bytes : valid) {
        EXPECT_TRUE(validator->validate(makeString(bytes)))
                << cb::hex_encode(bytes);
    }

    const std::vector<std::string> invalid = {
            "\x80", // Unexpected continuation byte
            "\xbf", // Unexpected continuation byte
            "\xc0\x80", // Overlong U+0000
            "\xc1\xbf", // Overlong U+007F
            "\xe0\x80\x80", // Overlong U+0000
            "\xe0\x9f\xbf", // Overlong U+07FF
            "\xed\xa0\x80", // Surrogate U+D800
            "\xed\xbf\xbf", // Surrogate U+DFFF
            "\xf0\x80\x80\x80", // Overlong U+0000
            "\xf0\x8f\xbf\xbf", // Overlong U+FFFF
            "\xf4\x90\x80\x80", // U+110000
            "\xf5\x80\x80\x80", // Above U+10FFFF
            "\xf7\xbf\xbf\xbf", // Above U+10FFFF
            "\xf8\x88\x80\x80\x80", // 5 byte sequence
            "\xfc\x84\x80\x80\x80\x80", // 6 byte sequence
            "\xfe",
            "\xff",
            "\xc3", // Truncated
            "\xe2\x82", // Truncated
            "\xf0\x9f\x98", // Truncated
            "\xc3\x28", // Invalid continuation byte
            "\xe2\x28\xac", // Invalid continuation byte
            "\xe2\x82\x28", // Invalid continuation byte
            "\xc3\xc3\xa9", // Lead byte inside of a sequence
            "\xe2\x82\xac\xac"}; // Unexpected continuation byte
    for (const auto& bytes : invalid) {
        EXPECT_FALSE(validator->validate(makeString(bytes)))
                << cb::hex_encode(bytes);
    }
}

/// The scalar and vectorized implementations must agree on the UTF-8
/// encoding of all (one and two byte) sequences, and of all lead bytes of
/// three and four byte sequences combined with all second bytes and the
/// boundary values of the following bytes
TEST_P(ValidatorTest, Utf8EquivalenceTest) {
    auto otherValidator =
            std::make_unique<JSON_checker::Validator>(!GetParam());
    auto check = [&](std::string_view bytes) {
        const auto s = makeString(bytes);
        EXPECT_EQ(validator->validate(s), otherValidator->validate(s))
                << "Validators do not agree when parsing "
                << cb::hex_encode(bytes);
    };

    for (int ii = 0x80; ii <= 0xff; ++ii) {
        for (int jj = 0; jj <= 0xff; ++jj) {
            check(std::string{char(ii), char(jj)});
        }
    }

    const std::array<char, 4> boundaries = {'\x7f', '\x80', '\xbf', '\xc0'};
    for (int ii = 0xe0; ii <= 0xff; ++ii) {
        for (int jj = 0; jj <= 0xff; ++jj) {
            for (const auto third : boundaries) {
                check(std::string{char(ii), char(jj), third});
                for (const auto fourth : boundaries) {
                    check(std::string{char(ii), char(jj), third, fourth});
                }
            }
        }
    }
}

/// Validating a document split in chunks (at any position, also within a
/// multi-byte UTF-8 sequence) must give the same result as validating it in
/// one piece
TEST_P(ValidatorTest, ChunksEquivalenceTest) {
    const std::vector<std::string> documents = {
            R"({"test": 12})",
            R"({"test": [1, 2.5e3, true, false, null, "x"]})",
            "\"\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80\"",
            R"(["\u00e9", "\n"])",
            "12",
            "-0.5E-3",
            "  true  ",
            "",
            " ",
            R"({"test": 12)",
            R"({"test": 12}})",
            "[1 2]",
            R"("\u12")",
            "\"\xff\"",
            "\"\xc3\x28\"",
            "\"\xc3\"",
            "\"\xe2\x82\"",
            "\"\x80\"",
            "\"\xed\x9f\xbf\xf4\x8f\xbf\xbf\"",
            "\"\xc0\x80\"",
            "\"\xe0\x9f\xbf\"",
            "\"\xed\xa0\x80\"",
            "\"\xf0\x8f\xbf\xbf\"",
            "\"\xf4\x90\x80\x80\"",
            "\"\xf5\x80\x80\x80\"",
            "12 13",
            "tru"};

    for (const auto& doc : documents) {
        const bool expected = validator->validate(doc);
        const std::string_view view = doc;
        EXPECT_EQ(expected, validator->validateChunks({view})) << doc;
        for (size_t ii = 0; ii <= doc.size(); ++ii) {
            for (size_t jj = ii; jj <= doc.size(); ++jj) {
                EXPECT_EQ(expected,
                          validator->validateChunks({view.substr(0, ii),
                                                     view.substr(ii, jj - ii),
                                                     view.substr(jj)}))
                        << doc << " split at " << ii << " and " << jj;
            }
        }
    }
}

TEST_P(ValidatorTest, ChunksTest) {
    EXPECT_FALSE(validator->validateChunks({}));
    EXPECT_TRUE(validator->validateChunks({"[", "1,2", "]"}));
    EXPECT_FALSE(validator->validateChunks({"[", "1 2", "]"}));
    EXPECT_TRUE(validator->validateChunks({R"({"k":)", "[1]", "}"}));
    EXPECT_FALSE(validator->validateChunks({R"({"k":)", R"(1,"j":2)", "]"}));
    EXPECT_TRUE(validator->validateChunks({"[\"", "key", "\"]"}));
    EXPECT_FALSE(validator->validateChunks({"[\"", "k\"ey", "\"]"}));
    EXPECT_FALSE(validator->validateChunks({"[\"", "\xc3", "\"]"}));
    // The validator is reset between validations
    EXPECT_FALSE(validator->validateChunks({"[", "1"}));
    EXPECT_TRUE(validator->validateChunks({"1", "2"}));
    EXPECT_TRUE(validator->validate("[]"));
}

namespace {

/** A UTF-8 sequence, and whether it is valid */
struct Utf8Sequence {
    std::string bytes;
    bool valid;
};

/** UTF-8 sequences at the edges of what is valid */
const std::vector<Utf8Sequence>& utf8Sequences() {
    static const std::vector<Utf8Sequence> sequences{
            // The smallest and largest of each length
            {"\xc2\x80", true},
            {"\xdf\xbf", true},
            {"\xe0\xa0\x80", true},
            {"\xef\xbf\xbf", true},
            {"\xf0\x90\x80\x80", true},
            {"\xf4\x8f\xbf\xbf", true},
            // Around the UTF-16 surrogates
            {"\xed\x9f\xbf", true},
            {"\xee\x80\x80", true},
            // Common characters (é, € and an emoji)
            {"\xc3\xa9", true},
            {"\xe2\x82\xac", true},
            {"\xf0\x9f\x98\x80", true},
            // Overlong encodings
            {"\xc0\x80", false},
            {"\xc1\xbf", false},
            {"\xe0\x9f\xbf", false},
            {"\xf0\x8f\xbf\xbf", false},
            // UTF-16 surrogates
            {"\xed\xa0\x80", false},
            {"\xed\xbf\xbf", false},
            // Above U+10FFFF and invalid lead bytes
            {"\xf4\x90\x80\x80", false},
            {"\xf5\x80\x80\x80", false},
            {"\xf8\x88\x80\x80\x80", false},
            {"\xff", false},
            {"\xfe", false},
            // Continuation bytes without a lead byte
            {"\x80", false},
            {"\xbf\xbf", false},
            // Truncated sequences
            {"\xc3", false},
            {"\xe2\x82", false},
            {"\xf0\x9f\x98", false},
            // A lead byte followed by something else than a continuation
            {"\xc3\x41", false},
            {"\xe2\x41\x82", false},
            {"\xc3\xc3\xa9", false},
            // Too many continuation bytes
            {"\xc3\xa9\xa9", false}};
    return sequences;
}

} // namespace

// UTF-8 sequences at different offsets (around the 16 byte blocks of the
// vectorized implementation), split between chunks at every position
TEST_P(ValidatorTest, ChunksUtf8Sequences) {
    for (const auto& [bytes, valid] : utf8Sequences()) {
        for (size_t prefix = 0; prefix < 40; ++prefix) {
            const std::string doc =
                    "\"" + std::string(prefix, 'x') + bytes + "yyy\"";
            const std::string_view view = doc;
            const auto hex = cb::hex_encode(bytes);
            EXPECT_EQ(valid, validator->validate(doc)) << hex << " " << prefix;
            EXPECT_EQ(valid, validator->validateChunks({view}))
                    << hex << " " << prefix;
            // Two and three chunks, split anywhere
            for (size_t ii = 0; ii <= doc.size(); ++ii) {
                EXPECT_EQ(valid,
                          validator->validateChunks(
                                  {view.substr(0, ii), view.substr(ii)}))
                        << hex << " " << prefix << " split at " << ii;
                for (size_t jj = ii; jj <= doc.size(); ++jj) {
                    EXPECT_EQ(
                            valid,
                            validator->validateChunks({view.substr(0, ii),
                                                       view.substr(ii, jj - ii),
                                                       view.substr(jj)}))
                            << hex << " " << prefix << " split at " << ii
                            << " and " << jj;
                }
            }
            // Every byte of the sequence in a chunk of its own (with empty
            // chunks in between)
            const size_t begin = 1 + prefix;
            std::vector<std::string_view> parts;
            for (size_t ii = 0; ii < bytes.size(); ++ii) {
                parts.push_back(view.substr(begin + ii, 1));
            }
            parts.resize(5);
            EXPECT_EQ(valid,
                      validator->validateChunks(
                              {view.substr(0, begin),
                               parts[0],
                               "",
                               parts[1],
                               parts[2],
                               "",
                               parts[3],
                               parts[4],
                               view.substr(begin + bytes.size())}))
                    << hex << " " << prefix << " one byte per chunk";
        }
    }
}

// Tokens split between chunks
TEST_P(ValidatorTest, ChunksSplitTokens) {
    // Escape sequences
    EXPECT_TRUE(validator->validateChunks({"\"a\\", "nb\""}));
    EXPECT_TRUE(validator->validateChunks({"\"a", "\\", "\"b\""}));
    EXPECT_TRUE(validator->validateChunks({"\"\\u00", "e9\""}));
    EXPECT_TRUE(validator->validateChunks({"\"\\", "u", "00E9", "\""}));
    EXPECT_FALSE(validator->validateChunks({"\"a\\", "qb\""}));
    EXPECT_FALSE(validator->validateChunks({"\"\\u00", "g9\""}));
    EXPECT_FALSE(validator->validateChunks({"\"\\u0", "0\""}));
    // Numbers and literals
    EXPECT_TRUE(validator->validateChunks({"1", "2"}));
    EXPECT_TRUE(validator->validateChunks({"[-", "1.5e", "+3]"}));
    EXPECT_TRUE(validator->validateChunks({"[tr", "ue,n", "ull,fa", "lse]"}));
    EXPECT_FALSE(validator->validateChunks({"[1.", "]"}));
    EXPECT_FALSE(validator->validateChunks({"[1e", "]"}));
    EXPECT_FALSE(validator->validateChunks({"[tr", "ux]"}));
    EXPECT_FALSE(validator->validateChunks({"[nul", "]"}));
    EXPECT_FALSE(validator->validateChunks({"0", "1"}));
    // Structure
    EXPECT_TRUE(validator->validateChunks({"{\"k\"", ":", "1}"}));
    EXPECT_TRUE(validator->validateChunks({"[1", ",", "2]"}));
    EXPECT_TRUE(validator->validateChunks({"[", "", "1", "", "]"}));
    EXPECT_TRUE(validator->validateChunks({" ", "[1]", " "}));
    EXPECT_FALSE(validator->validateChunks({"[1,", "]"}));
    EXPECT_FALSE(validator->validateChunks({"[1", "]]"}));
    EXPECT_FALSE(validator->validateChunks({"{\"k\"", "1}"}));
    EXPECT_FALSE(validator->validateChunks({"{\"k\":1", ",}"}));
    EXPECT_FALSE(validator->validateChunks({"[", "1", "]", "x"}));
    EXPECT_FALSE(validator->validateChunks({"[1]", "[2]"}));
    // Strings
    EXPECT_TRUE(validator->validateChunks({"\"", "", "\""}));
    EXPECT_FALSE(validator->validateChunks({"\"abc"}));
    EXPECT_FALSE(validator->validateChunks({"\"abc", ""}));
    EXPECT_FALSE(validator->validateChunks({"\"a", "\n\""}));
    EXPECT_FALSE(validator->validateChunks(
            {"\"a", std::string_view("\0", 1), "\""}));
    // Nothing but empty chunks or whitespace
    EXPECT_FALSE(validator->validateChunks({"", ""}));
    EXPECT_FALSE(validator->validateChunks({" ", "\n"}));
}

// An escape sequence, a control character or a multi-byte character at
// every position of strings long enough for the vectorized implementation,
// split between two chunks at every position
TEST_P(ValidatorTest, ChunksLongStrings) {
    const std::vector<std::pair<std::string, bool>> pieces{
            {"\\n", true},
            {"\\\"", true},
            {"\\\\", true},
            {"\\u20AC", true},
            {"\xe2\x82\xac", true},
            {"\"", false},
            {"\x01", false},
            {"\x1f", false},
            {"\\x", false},
            {"\xe2\x82", false}};
    for (const auto& [piece, valid] : pieces) {
        for (size_t length = 0; length < 100; length += 7) {
            for (size_t pos = 0; pos <= length; ++pos) {
                const std::string doc = "[\"" + std::string(pos, 'a') + piece +
                                        std::string(length - pos, 'b') + "\"]";
                const std::string_view view = doc;
                ASSERT_EQ(valid, validator->validate(doc)) << doc;
                for (size_t ii = 0; ii <= doc.size(); ++ii) {
                    EXPECT_EQ(valid,
                              validator->validateChunks(
                                      {view.substr(0, ii), view.substr(ii)}))
                            << doc << " split at " << ii;
                }
            }
        }
    }
}

namespace {

/** Append a random JSON value (which may be made invalid by the caller) */
void appendRandomJsonValue(std::mt19937& gen, std::string& out, int depth) {
    switch (gen() % (depth > 3 ? 4 : 6)) {
    case 0:
        out += std::to_string(int(gen() % 2000) - 1000);
        return;
    case 1:
        out += gen() % 2 ? "true" : "-1.25e-3";
        return;
    case 2:
        out += "null";
        return;
    case 3: {
        // A string, mostly with valid UTF-8
        out += '"';
        const auto length = gen() % 40;
        for (size_t ii = 0; ii < length; ++ii) {
            switch (gen() % 10) {
            case 0:
                out += "\\\"";
                break;
            case 1: {
                const auto& sequences = utf8Sequences();
                const auto& sequence = sequences[gen() % sequences.size()];
                if (sequence.valid || gen() % 8 == 0) {
                    out += sequence.bytes;
                }
                break;
            }
            default:
                out += char('a' + (gen() % 26));
            }
        }
        out += '"';
        return;
    }
    case 4: {
        out += '[';
        const auto length = gen() % 4;
        for (size_t ii = 0; ii < length; ++ii) {
            out += ii ? "," : "";
            appendRandomJsonValue(gen, out, depth + 1);
        }
        out += ']';
        return;
    }
    default: {
        out += '{';
        const auto length = gen() % 4;
        for (size_t ii = 0; ii < length; ++ii) {
            out += ii ? ",\"k\":" : "\"k\":";
            appendRandomJsonValue(gen, out, depth + 1);
        }
        out += '}';
        return;
    }
    }
}

} // namespace

// Random documents (valid and invalid) split in random places are accepted
// exactly as the whole document is by the scalar implementation
TEST_P(ValidatorTest, ChunksRandom) {
    std::mt19937 gen(4242);
    JSON_checker::Validator reference(false);
    size_t numValid = 0;
    for (int ii = 0; ii < 50000; ++ii) {
        std::string doc;
        appendRandomJsonValue(gen, doc, 0);
        if (gen() % 3 == 0 && !doc.empty()) {
            // Change a byte to make the document (likely) invalid
            constexpr std::string_view replacements = ",:{}[]\"\\1 \x80\xc3";
            doc[gen() % doc.size()] = replacements[gen() % replacements.size()];
        }
        const bool expected = reference.validate(doc);
        numValid += expected ? 1 : 0;
        const std::string_view view = doc;
        const size_t first = gen() % (doc.size() + 1);
        const size_t second = first + (gen() % (doc.size() - first + 1));
        ASSERT_EQ(expected,
                  validator->validateChunks({view.substr(0, first),
                                             view.substr(first, second - first),
                                             view.substr(second)}))
                << cb::hex_encode(doc) << " split at " << first << " and "
                << second;
    }
    // Make sure both kinds of documents were generated
    EXPECT_GT(numValid, 10000);
    EXPECT_LT(numValid, 45000);
}

INSTANTIATE_TEST_SUITE_P(JSON_checker,
                         ValidatorTest,
                         ::testing::Bool(),
                         ::testing::PrintToStringParamName());

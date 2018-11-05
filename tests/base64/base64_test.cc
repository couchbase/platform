/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#include <folly/portability/GTest.h>
#include <platform/base64.h>

class Base64Test : public ::testing::Test {
protected:
    void validate(const std::string& source, const std::string& result) {
        std::string encoded;
        std::string decoded;

        ASSERT_NO_THROW(encoded = Couchbase::Base64::encode(source));
        EXPECT_EQ(result, encoded);

        ASSERT_NO_THROW(decoded = Couchbase::Base64::decode(encoded));
        EXPECT_EQ(source, decoded);
    }
};

TEST_F(Base64Test, testRFC4648) {
    validate("", "");
    validate("f", "Zg==");
    validate("fo", "Zm8=");
    validate("foo", "Zm9v");
    validate("foob", "Zm9vYg==");
    validate("fooba", "Zm9vYmE=");
    validate("foobar", "Zm9vYmFy");
}

TEST_F(Base64Test, testWikipediaExample) {
    /* Examples from http://en.wikipedia.org/wiki/Base64 */
    validate("Man is distinguished, not only by his reason, but by this "
                 "singular passion from other animals, which is a lust of "
                 "the mind, that by a perseverance of delight in the "
                 "continued and indefatigable generation of knowledge, "
                 "exceeds the short vehemence of any carnal pleasure.",
             "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24s"
                 "IGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBh"
                 "bmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQg"
                 "YnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"
                 "dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xl"
                 "ZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNh"
                 "cm5hbCBwbGVhc3VyZS4=");
    validate("pleasure.", "cGxlYXN1cmUu");
    validate("leasure.", "bGVhc3VyZS4=");
    validate("easure.", "ZWFzdXJlLg==");
    validate("asure.", "YXN1cmUu");
    validate("sure.", "c3VyZS4=");
}

TEST_F(Base64Test, testStuff) {
    // Dummy test data. It looks like the "base64" command line
    // utility from gnu coreutils adds the "\n" to the encoded data...
    validate("Administrator:password", "QWRtaW5pc3RyYXRvcjpwYXNzd29yZA==");
    validate("@", "QA==");
    validate("@\n", "QAo=");
    validate("@@", "QEA=");
    validate("@@\n", "QEAK");
    validate("@@@", "QEBA");
    validate("@@@\n", "QEBACg==");
    validate("@@@@", "QEBAQA==");
    validate("@@@@\n", "QEBAQAo=");
    validate("blahblah:bla@@h", "YmxhaGJsYWg6YmxhQEBo");
    validate("blahblah:bla@@h\n", "YmxhaGJsYWg6YmxhQEBoCg==");
}

TEST_F(Base64Test, TestDecode) {
    std::vector<uint8_t> salt{0x41, 0x25, 0xc2, 0x47, 0xe4, 0x3a, 0xb1, 0xe9,
                              0x3c, 0x6d, 0xff, 0x76};

    validate(std::string(reinterpret_cast<char*>(salt.data()), salt.size()),
             "QSXCR+Q6sek8bf92");
}

TEST_F(Base64Test, TestPrettyPrint) {
    std::string input(
            "Man is distinguished, not only by his reason, but by this "
            "singular passion from other animals, which is a lust of "
            "the mind, that by a perseverance of delight in the "
            "continued and indefatigable generation of knowledge, "
            "exceeds the short vehemence of any carnal pleasure.");
    std::string output(
            "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1\n"
            "dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3\n"
            "aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFu\n"
            "Y2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxl\n"
            "IGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhl\n"
            "bWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=\n");

    EXPECT_EQ(output, cb::base64::encode(input, true));

    auto decoded = cb::base64::decode(output);
    EXPECT_EQ(input,
              std::string(reinterpret_cast<const char*>(decoded.data()),
                          decoded.size()));
}

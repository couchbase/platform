/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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
#include <gtest/gtest.h>

#include <map>
#include <extmeta/extmeta.h>
#include <stdexcept>

TEST(CbMetaBuilder, TestEmpty) {
    cb::extmeta::Builder builder;
    builder.encode();
    EXPECT_EQ(0, builder.getEncodedData().size());
}

TEST(CbMetaBuilder, TestEmptyMissingEncode) {
    cb::extmeta::Builder builder;
    EXPECT_THROW(builder.getEncodedData(),
                std::logic_error);
}

TEST(CbMetaBuilder, TestEmptyDoubleEncode) {
    cb::extmeta::Builder builder;
    builder.encode();
    EXPECT_THROW(builder.encode(),
                 std::logic_error);
}

TEST(CbMetaBuilder, TestEmptyEncodeBeforeAdd) {
    cb::extmeta::Builder builder;
    builder.encode();
    EXPECT_THROW(builder.add(cb::extmeta::Type::ADJUSTED_TIME, 32),
                 std::logic_error);
}

TEST(CbMetaBuilder, ChainingWorks) {
    cb::extmeta::Builder builder;
    std::string foo{"hallo"};
    builder.add(cb::extmeta::Type::CONFLICT_RES_MODE, foo)
           .add(cb::extmeta::Type::ADJUSTED_TIME, foo);
    builder.encode();
    const size_t size = 1 /* magic */ +
        (2 * (1 /* type */ + 2 /* length */ + 5 /* value */));
    EXPECT_EQ(size, builder.getEncodedData().size());
}

TEST(CbMetaBuilder, OnlyOneInstanceOfEachType) {
    cb::extmeta::Builder builder;
    std::string foo{"hallo"};
    builder.add(cb::extmeta::Type::CONFLICT_RES_MODE, foo);
    EXPECT_THROW(builder.add(cb::extmeta::Type::CONFLICT_RES_MODE, foo),
                 std::logic_error);
}

TEST(CbMetaBuilder, ReservedRange) {
    cb::extmeta::Builder builder;
    std::string foo{"hallo"};
    for (int ii = 0; ii < 256; ++ii) {
        if (ii < 0x80) {
            EXPECT_NO_THROW(builder.add(cb::extmeta::Type(ii),
                                        reinterpret_cast<uint8_t*>(&ii), 1));
        } else {
            EXPECT_THROW(builder.add(cb::extmeta::Type(ii),
                                     reinterpret_cast<uint8_t*>(&ii), 1),
                         std::logic_error);
        }
    }
}

TEST(CbMetaBuilder, TestInitializeWithMeta) {
    std::vector<uint8_t> expected;
    expected.push_back(1); // version
    expected.push_back(1); // adjusted time type
    expected.push_back(0); // length MSB
    expected.push_back(4); // length LSB
    expected.push_back(0); // 32 bit integer MSB
    expected.push_back(0); // 32 bit integer MSB - 1
    expected.push_back(0); // 32 bit integer LSB + 1
    expected.push_back(32); // 32 bit integer LSB

    // Initialize with a buffer
    cb::extmeta::Builder builder(expected);
    builder.add(cb::extmeta::Type::CONFLICT_RES_MODE, 10);
    builder.encode();

    expected.push_back(2); // conflict res mode type
    expected.push_back(0); // length MSB
    expected.push_back(4); // length LSB
    expected.push_back(0); // 32 bit integer MSB
    expected.push_back(0); // 32 bit integer MSB - 1
    expected.push_back(0); // 32 bit integer LSB + 1
    expected.push_back(10); // 32 bit integer LSB

    EXPECT_EQ(expected, builder.getEncodedData());
}

TEST(CbMetaBuilder, TestInitializeWithInvalidMeta) {
    std::vector<uint8_t> expected;
    expected.push_back(1); // version
    expected.push_back(1); // adjusted time type
    expected.push_back(0); // length MSB
    expected.push_back(4); // length LSB
    expected.push_back(0); // 32 bit integer MSB
    expected.push_back(0); // 32 bit integer MSB - 1
    expected.push_back(0); // 32 bit integer LSB + 1
    expected.push_back(32); // 32 bit integer LSB

    expected.push_back(1); // Add a type, that would require more data
    EXPECT_THROW(cb::extmeta::Builder builder(expected),
                 std::underflow_error);

    // We should not be able to have duplicate types in the stream..
    // complete the duplicate entry)
    expected.push_back(0); // length MSB
    expected.push_back(4); // length LSB
    expected.push_back(0); // 32 bit integer MSB
    expected.push_back(0); // 32 bit integer MSB - 1
    expected.push_back(0); // 32 bit integer LSB + 1
    expected.push_back(23); // 32 bit integer LSB
    EXPECT_THROW(cb::extmeta::Builder builder(expected),
                 std::logic_error);
}


TEST(CbMetaBuilder, TestHelperAdd16bit) {
    std::vector<uint8_t> expected;
    expected.push_back(1); // version
    expected.push_back(1); // adjusted time type
    expected.push_back(0); // length MSB
    expected.push_back(2); // length LSB
    expected.push_back(0); // 16 bit integer MSB
    expected.push_back(10); // 16 bit integer LSB

    cb::extmeta::Builder uint16_builder;
    uint16_builder.add(cb::extmeta::Type::ADJUSTED_TIME, uint16_t(10)).encode();
    EXPECT_EQ(expected, uint16_builder.getEncodedData());

    cb::extmeta::Builder int16_builder;
    int16_builder.add(cb::extmeta::Type::ADJUSTED_TIME, int16_t(-10)).encode();

    // remove the value, and put in the negative value
    expected.resize(expected.size() - 2);
    expected.push_back(0xff);
    expected.push_back(0xf6);

    EXPECT_EQ(expected, int16_builder.getEncodedData());
}

TEST(CbMetaBuilder, TestHelperAddInt32) {
    std::vector<uint8_t> expected;
    expected.push_back(1); // version
    expected.push_back(1); // adjusted time type
    expected.push_back(0); // length MSB
    expected.push_back(4); // length LSB
    expected.push_back(0); // 32 bit integer MSB
    expected.push_back(0); // 32 bit integer MSB - 1
    expected.push_back(0); // 32 bit integer MSB - 2
    expected.push_back(10); // 32 bit integer LSB

    cb::extmeta::Builder uint32_builder;
    uint32_builder.add(cb::extmeta::Type::ADJUSTED_TIME, uint32_t(10)).encode();
    EXPECT_EQ(expected, uint32_builder.getEncodedData());

    cb::extmeta::Builder int32_builder;
    int32_builder.add(cb::extmeta::Type::ADJUSTED_TIME, int32_t(-10)).encode();

    // remove the value, and put in the negative value
    expected.resize(expected.size() - 4);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xf6);

    EXPECT_EQ(expected, int32_builder.getEncodedData());
}

TEST(CbMetaBuilder, TestHelperAddInt64) {
    std::vector<uint8_t> expected;
    expected.push_back(1); // version
    expected.push_back(1); // adjusted time type
    expected.push_back(0); // length MSB
    expected.push_back(8); // length LSB
    expected.push_back(0); // 64 bit integer MSB
    expected.push_back(0); // 64 bit integer MSB - 1
    expected.push_back(0); // 64 bit integer MSB - 2
    expected.push_back(0); // 64 bit integer MSB - 3
    expected.push_back(0); // 64 bit integer LSB + 3
    expected.push_back(0); // 64 bit integer LSB + 2
    expected.push_back(0); // 64 bit integer LSB + 1
    expected.push_back(10); // 64 bit integer LSB

    cb::extmeta::Builder uint64_builder;
    uint64_builder.add(cb::extmeta::Type::ADJUSTED_TIME, uint64_t(10)).encode();
    EXPECT_EQ(expected, uint64_builder.getEncodedData());

    cb::extmeta::Builder int64_builder;
    int64_builder.add(cb::extmeta::Type::ADJUSTED_TIME, int64_t(-10)).encode();

    // remove the value, and put in the negative value
    expected.resize(expected.size() - 8);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xff);
    expected.push_back(0xf6);

    EXPECT_EQ(expected, int64_builder.getEncodedData());
}

/**
 * The NotUsedReceiver is a receiver which throws an exception
 * if it receives any data. It is used by the tests that should throw
 * an exception before any data is found
 */
class NotUsedReceiver : public cb::extmeta::Parser::Receiver {
public:
    bool add(const cb::extmeta::Type& type,
             const uint8_t* data,
             size_t size) override {
        throw std::runtime_error("NotUsedReceiver::add: should not be called");
    }
};

TEST(CbMetaParser, TestNoData) {
    NotUsedReceiver receiver;
    EXPECT_NO_THROW(cb::extmeta::Parser::parse(nullptr, 0, receiver));
}

TEST(CbMetaParser, TestCorrectVersionNoData) {
    std::vector<uint8_t> data(0);
    data.push_back(uint8_t(cb::extmeta::Version::ONE));
    NotUsedReceiver receiver;
    EXPECT_NO_THROW(cb::extmeta::Parser::parse(data.data(), data.size(),
                                            receiver));
}

TEST(CbMetaParser, TestInvalidVersionNoData) {
    std::vector<uint8_t> data(0);
    data.push_back(uint8_t(cb::extmeta::Version::ONE) + 10);
    NotUsedReceiver receiver;
    EXPECT_THROW(cb::extmeta::Parser::parse(data.data(), data.size(), receiver),
                 std::invalid_argument);
}

TEST(CbMetaParser, TestMissingLength) {
    std::vector<uint8_t> data(0);
    data.push_back(uint8_t(cb::extmeta::Version::ONE));
    data.push_back(1); // Add a type of 1
    NotUsedReceiver receiver;
    EXPECT_THROW(cb::extmeta::Parser::parse(data.data(), data.size(), receiver),
                 std::underflow_error);
}

TEST(CbMetaParser, TestMissingBytes) {
    std::vector<uint8_t> data(0);
    data.push_back(uint8_t(cb::extmeta::Version::ONE));
    data.push_back(1); // Add a type of 1
    data.push_back(0); // length MSB
    data.push_back(4); // length LSB
    NotUsedReceiver receiver;
    EXPECT_THROW(cb::extmeta::Parser::parse(data.data(), data.size(), receiver),
                 std::underflow_error);
}

/**
 * The MyReceiver class is used by the tests to cache all of the individual
 * meta data tags found in the blob, and allow for terminating the parsing
 * when a certain type is found.
 */
class MyReceiver : public cb::extmeta::Parser::Receiver {
public:
    MyReceiver(uint8_t stop = 0xff)
        : stoptype(cb::extmeta::Type(stop)) {
    }

    bool add(const cb::extmeta::Type& type,
             const uint8_t* data,
             size_t size) override {

        entrymap[uint8_t(type)] = std::string{reinterpret_cast<const char*>(data),
                                              size};
        return type != stoptype;
    }

    std::map<uint8_t, std::string> entrymap;

private:
    cb::extmeta::Type stoptype;
};


TEST(CbMetaParser, TestSingleEntry) {
    cb::extmeta::Builder builder;
    builder.add(cb::extmeta::Type(0x20), std::string{"hallo"}).encode();
    const auto& data = builder.getEncodedData();
    MyReceiver receiver;
    cb::extmeta::Parser::parse(data.data(), data.size(), receiver);

    EXPECT_EQ(1, receiver.entrymap.size());
    EXPECT_EQ("hallo", receiver.entrymap[0x20]);
}

TEST(CbMetaParser, TestMultipleEntry) {
    cb::extmeta::Builder builder;
    builder.add(cb::extmeta::Type(0x20), std::string{"hallo"})
           .add(cb::extmeta::Type(0x21), std::string{"world"})
           .encode();
    const auto& data = builder.getEncodedData();

    MyReceiver receiver;
    cb::extmeta::Parser::parse(data.data(), data.size(), receiver);

    EXPECT_EQ(2, receiver.entrymap.size());
    EXPECT_EQ("hallo", receiver.entrymap[0x20]);
    EXPECT_EQ("world", receiver.entrymap[0x21]);
}

// Validate that we can tell the parser to stop when we tell it to
TEST(CbMetaParser, TestMultipleEntryPartialParse) {
    cb::extmeta::Builder builder;
    builder.add(cb::extmeta::Type(0x20), std::string{"hallo"})
           .add(cb::extmeta::Type(0x21), std::string{"world"})
           .encode();
    const auto& data = builder.getEncodedData();

    MyReceiver receiver(0x20);
    cb::extmeta::Parser::parse(data.data(), data.size(), receiver);

    EXPECT_EQ(1, receiver.entrymap.size());
    EXPECT_EQ("hallo", receiver.entrymap[0x20]);
}

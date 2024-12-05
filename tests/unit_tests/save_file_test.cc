/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>

#include <platform/dirutils.h>

class SaveFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        filename = cb::io::mktemp("savefiletest");
    }
    void TearDown() override {
        permissions(filename,
                    std::filesystem::perms::owner_write |
                            std::filesystem::perms::group_write |
                            std::filesystem::perms::others_write,
                    std::filesystem::perm_options::add);
        remove_all(filename);
    }
    std::filesystem::path filename;
};

TEST_F(SaveFileTest, SaveOk) {
    cb::io::saveFile(filename, "Hello");
    EXPECT_EQ("Hello", cb::io::loadFile(filename));
}

TEST_F(SaveFileTest, SaveOkNoThrow) {
    std::error_code ec;
    EXPECT_TRUE(cb::io::saveFile(filename, "Hello", ec));
}

TEST_F(SaveFileTest, TestThrowingVersion) {
    permissions(filename,
                std::filesystem::perms::owner_write |
                        std::filesystem::perms::group_write |
                        std::filesystem::perms::others_write,
                std::filesystem::perm_options::remove);

    try {
        cb::io::saveFile(filename, "Hello");
        FAIL() << "Should have thrown";
    } catch (const std::system_error& e) {
        EXPECT_EQ(std::errc::operation_not_permitted,
                  static_cast<std::errc>(e.code().value()))
                << e.code().value();
    } catch (const std::exception& e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(SaveFileTest, TestNoThrowingVersion) {
    permissions(filename,
                std::filesystem::perms::owner_write |
                        std::filesystem::perms::group_write |
                        std::filesystem::perms::others_write,
                std::filesystem::perm_options::remove);

    std::error_code ec;
    EXPECT_FALSE(cb::io::saveFile(filename, "Hello", ec));
    EXPECT_TRUE(ec);
    EXPECT_EQ(std::errc::operation_not_permitted,
              static_cast<std::errc>(ec.value()))
            << ec.value();
}

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
    EXPECT_EQ("Hello", cb::io::loadFile(filename.string()));
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
        EXPECT_EQ(std::errc::permission_denied, e.code()) << e.what();
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
    EXPECT_EQ(std::errc::permission_denied, ec) << ec.message();
}

TEST_F(SaveFileTest, SaveAtomicCreatesFile) {
    remove(filename);
    cb::io::saveFileAtomic(filename, "Hello");
    EXPECT_EQ("Hello", cb::io::loadFile(filename.string()));
    auto tempfile = filename;
    tempfile += ".tmp";
    EXPECT_FALSE(exists(tempfile));
}

TEST_F(SaveFileTest, SaveAtomicReplacesFile) {
    cb::io::saveFile(filename, "Hello world");
    cb::io::saveFileAtomic(filename, "Hi");
    EXPECT_EQ("Hi", cb::io::loadFile(filename.string()));
}

TEST_F(SaveFileTest, SaveAtomicEmptyContent) {
    cb::io::saveFile(filename, "Hello");
    std::error_code ec;
    EXPECT_TRUE(cb::io::saveFileAtomic(filename, {}, ec)) << ec.message();
    EXPECT_TRUE(cb::io::loadFile(filename.string()).empty());
}

/// A failure to write the new content must leave the old file untouched
/// and not leave the temporary file behind
TEST_F(SaveFileTest, SaveAtomicFailureKeepsOriginal) {
    cb::io::saveFile(filename, "Hello");
    // Block the temporary file by making it a non-empty directory
    auto tempfile = filename;
    tempfile += ".tmp";
    create_directories(tempfile / "blocker");

    std::error_code ec;
    EXPECT_FALSE(cb::io::saveFileAtomic(filename, "World", ec));
    EXPECT_TRUE(ec);
    EXPECT_EQ("Hello", cb::io::loadFile(filename.string()));
    EXPECT_THROW(cb::io::saveFileAtomic(filename, "World"), std::system_error);
    EXPECT_EQ("Hello", cb::io::loadFile(filename.string()));
    remove_all(tempfile);
}

TEST_F(SaveFileTest, FsyncDirectory) {
    cb::io::fsyncDirectory(filename.parent_path());
    cb::io::fsyncDirectory({});
#ifndef WIN32
    auto missing = filename;
    missing += ".missing";
    EXPECT_THROW(cb::io::fsyncDirectory(missing), std::system_error);
#endif
}

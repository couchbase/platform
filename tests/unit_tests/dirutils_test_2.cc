/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/dirutils.h>

class DirnameTest : public ::testing::Test {};

TEST_F(DirnameTest, HandleEmptyString) {
    EXPECT_EQ(".", cb::io::dirname(""));
}

TEST_F(DirnameTest, HandleNoDirectorySeparator) {
    EXPECT_EQ(".", cb::io::dirname("foo"));
}

TEST_F(DirnameTest, HandleRootDirectory) {
    EXPECT_EQ("\\", cb::io::dirname("\\foo"));
    EXPECT_EQ("/", cb::io::dirname("/foo"));
}

TEST_F(DirnameTest, HandleSingleDirectory) {
    EXPECT_EQ("foo", cb::io::dirname("foo\\bar"));
    EXPECT_EQ("foo", cb::io::dirname("foo/bar"));
}

TEST_F(DirnameTest, HandleRootedSingleDirectory) {
    EXPECT_EQ("\\foo", cb::io::dirname("\\foo\\bar"));
    EXPECT_EQ("/foo", cb::io::dirname("/foo/bar"));
}

TEST_F(DirnameTest, HandleTwolevelDirectory) {
    EXPECT_EQ("foo\\bar", cb::io::dirname("foo\\bar\\foobar"));
    EXPECT_EQ("foo/bar", cb::io::dirname("foo/bar/foobar"));
}

TEST_F(DirnameTest, HandleRootedTwolevelDirectory) {
    EXPECT_EQ("\\foo\\bar", cb::io::dirname("\\foo\\bar\\foobar"));
    EXPECT_EQ("/foo/bar", cb::io::dirname("/foo/bar/foobar"));
}

class BasenameTest : public ::testing::Test {};

TEST_F(BasenameTest, HandleEmptyString) {
    EXPECT_EQ("", cb::io::basename(""));
}

TEST_F(BasenameTest, HandleNoDirectory) {
    EXPECT_EQ("foo", cb::io::basename("foo"));
}

TEST_F(BasenameTest, HandleRootDirectory) {
    EXPECT_EQ("foo", cb::io::basename("\\foo"));
    EXPECT_EQ("foo", cb::io::basename("/foo"));
}

TEST_F(BasenameTest, HandleSingleDirectory) {
    EXPECT_EQ("bar", cb::io::basename("foo\\bar"));
    EXPECT_EQ("bar", cb::io::basename("foo/bar"));
}

TEST_F(BasenameTest, HandleRootedSingleDirectory) {
    EXPECT_EQ("bar", cb::io::basename("\\foo\\bar"));
    EXPECT_EQ("bar", cb::io::basename("/foo/bar"));
}

TEST_F(BasenameTest, HandleTwoLevelDirectory) {
    EXPECT_EQ("foobar", cb::io::basename("foo\\bar\\foobar"));
    EXPECT_EQ("foobar", cb::io::basename("foo/bar/foobar"));
}

TEST_F(BasenameTest, HandleRootedTwoLevelDirectory) {
    EXPECT_EQ("foobar", cb::io::basename("\\foo\\bar\\foobar"));
    EXPECT_EQ("foobar", cb::io::basename("/foo/bar/foobar"));
}

class DiskMatchingTest : public ::testing::Test {
public:
    DiskMatchingTest() {
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/a.0"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/a.1"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/a.2"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/a.3"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/b.0"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/b.1"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/c.0"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/c.1"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/0.couch"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/0.couch.0"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/0.couch.2"));
        files.emplace_back(
                cb::io::sanitizePath("my-dirutil-test/3.couch.compact"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/1.couch"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/2.couch"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/3.couch"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/4.couch"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/5.couch"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/w1"));
        files.emplace_back(cb::io::sanitizePath("my-dirutil-test/w2"));
    }

    void SetUp() override {
        if (cb::io::isDirectory("my-dirutil-test")) {
            cb::io::rmrf("my-dirutil-test");
        }
        cb::io::mkdirp("my-dirutil-test");
        for (const auto& file : files) {
            ASSERT_TRUE(touch(file));
        }
    }

    void TearDown() override {
        cb::io::rmrf("my-dirutil-test");
    }

protected:
    bool inList(const std::vector<std::string>& list, const std::string& name) {
        const auto n =
                cb::io::makeExtendedLengthPath(cb::io::sanitizePath(name))
                        .generic_string();
        return std::find(list.begin(), list.end(), n) != list.end();
    }

    bool touch(const std::string& name) {
        FILE* fp = fopen(name.c_str(), "w");
        if (fp != nullptr) {
            fclose(fp);
            return true;
        }
        return false;
    }

    std::vector<std::string> files;
};

TEST_F(DiskMatchingTest, NonExistingDirectory) {
    EXPECT_TRUE(cb::io::findFilesWithPrefix("my-nonexisting", "dir").empty());
    EXPECT_TRUE(cb::io::findFilesWithPrefix("my-nonexisting/dir").empty());
}

TEST_F(DiskMatchingTest, findAllFiles) {
    auto f1 = cb::io::findFilesWithPrefix("my-dirutil-test", "");
    EXPECT_LE(files.size(), f1.size());

    for (const auto& ii : files) {
        EXPECT_TRUE(inList(f1, ii));
    }

    auto f2 = cb::io::findFilesWithPrefix("my-dirutil-test/");
    EXPECT_LE(files.size(), f2.size());
    for (const auto& ii : files) {
        EXPECT_TRUE(inList(f2, ii));
    }
}

TEST_F(DiskMatchingTest, findA0) {
    auto f1 = cb::io::findFilesWithPrefix("my-dirutil-test", "a.0");
    EXPECT_EQ(1, f1.size());

    auto f2 = cb::io::findFilesWithPrefix("my-dirutil-test/a.0");
    EXPECT_EQ(1, f2.size());
}

TEST_F(DiskMatchingTest, findAllA) {
    auto f1 = cb::io::findFilesWithPrefix("my-dirutil-test", "a");
    EXPECT_EQ(4, f1.size());

    EXPECT_TRUE(inList(f1, "my-dirutil-test/a.0"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/a.1"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/a.2"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/a.3"));

    auto f2 = cb::io::findFilesWithPrefix("my-dirutil-test/a");
    EXPECT_EQ(4, f2.size());

    EXPECT_TRUE(inList(f2, "my-dirutil-test/a.0"));
    EXPECT_TRUE(inList(f2, "my-dirutil-test/a.1"));
    EXPECT_TRUE(inList(f2, "my-dirutil-test/a.2"));
    EXPECT_TRUE(inList(f2, "my-dirutil-test/a.3"));
}

TEST_F(DiskMatchingTest, matchNoDirSubString) {
    auto f1 = cb::io::findFilesContaining("", "");
    EXPECT_EQ(0, f1.size());
}

TEST_F(DiskMatchingTest, matchEmptySubString) {
    auto f1 = cb::io::findFilesContaining("my-dirutil-test", "");
    EXPECT_LE(files.size(), f1.size());
}

TEST_F(DiskMatchingTest, matchSingleCharSubString) {
    auto f1 = cb::io::findFilesContaining("my-dirutil-test", "w");
    EXPECT_EQ(2, f1.size());
    EXPECT_TRUE(inList(f1, "my-dirutil-test/w1"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/w2"));
}

TEST_F(DiskMatchingTest, matchLongerSubString) {
    auto f1 = cb::io::findFilesContaining("my-dirutil-test", "couch");
    EXPECT_EQ(9, f1.size());

    EXPECT_TRUE(inList(f1, "my-dirutil-test/0.couch"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/0.couch.0"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/0.couch.2"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/3.couch.compact"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/1.couch"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/2.couch"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/3.couch"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/4.couch"));
    EXPECT_TRUE(inList(f1, "my-dirutil-test/5.couch"));
}

TEST_F(DiskMatchingTest, matchTailSubString) {
    auto f1 = cb::io::findFilesContaining("my-dirutil-test", "compact");
    EXPECT_EQ(1, f1.size());
    EXPECT_TRUE(inList(f1, "my-dirutil-test/3.couch.compact"));
}

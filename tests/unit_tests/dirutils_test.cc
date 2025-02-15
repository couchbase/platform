/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>

#include <platform/dirutils.h>
#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#endif

static bool CreateDirectory(const std::filesystem::path& dir) {
    try {
        create_directories(dir);
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

class IoTest : public ::testing::Test {
public:

    static void SetUpTestCase();

    static void TearDownTestCase();

protected:

    // A small filesystem we can play around with
    static std::vector<std::string> vfs;
};

std::vector<std::string> IoTest::vfs;

void IoTest::SetUpTestCase() {
    for (const auto& file : std::vector<std::string>{{"fs"},
                                                     {"fs/d1"},
                                                     {"fs/d2"},
                                                     {"fs/e2"},
                                                     {"fs/f2c"},
                                                     {"fs/g2"},
                                                     {"fs/d3"},
                                                     {"fs/1"},
                                                     {"fs/2"},
                                                     {"fs/2c"},
                                                     {"fs/2d"},
                                                     {"fs/3"},
                                                     {"fs/d1/d1"}}) {
        ASSERT_TRUE(CreateDirectory(file)) << "failed to create directory";
        vfs.emplace_back(cb::io::sanitizePath(file));
    }
}

void IoTest::TearDownTestCase() {
    // Expecting the rmrf to work ;)
    cb::io::rmrf("fs");
}

TEST_F(IoTest, findFilesWithPrefix) {
    auto vec = cb::io::findFilesWithPrefix("fs");
    EXPECT_EQ(1u, vec.size());

    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("./fs")));

    vec = cb::io::findFilesWithPrefix("fs", "d");
    EXPECT_EQ(3u, vec.size());

    // We don't know the order of the files in the result..
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/d1")));
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/d2")));
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/d3")));

    vec = cb::io::findFilesWithPrefix("fs", "1");
    EXPECT_EQ(1u, vec.size());
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/1")));

    vec = cb::io::findFilesWithPrefix("fs", "");
    EXPECT_EQ(vfs.size() - 2, vec.size());
}

TEST_F(IoTest, findFilesContaining) {
    auto vec = cb::io::findFilesContaining("fs", "2");
    EXPECT_EQ(7u, vec.size());
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/d2")));
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/e2")));
    EXPECT_NE(
            vec.end(),
            std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/f2c")));
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/g2")));
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/2")));
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/2c")));
    EXPECT_NE(vec.end(),
              std::find(vec.begin(), vec.end(), cb::io::sanitizePath("fs/2d")));
}

TEST_F(IoTest, mktemp) {
    auto filename = cb::io::mktemp("foo");
    EXPECT_FALSE(filename.empty())
                << "FAIL: Expected to create tempfile without mask";
    EXPECT_TRUE(cb::io::isFile(filename));
    cb::io::rmrf(filename);
    EXPECT_FALSE(cb::io::isFile(filename));
    EXPECT_FALSE(cb::io::isDirectory(filename));

    filename = cb::io::mktemp("barXXXXXX");
    EXPECT_FALSE(filename.empty())
                << "FAIL: Expected to create tempfile mask";
    EXPECT_TRUE(cb::io::isFile(filename));
    cb::io::rmrf(filename);
    EXPECT_FALSE(cb::io::isFile(filename));
    EXPECT_FALSE(cb::io::isDirectory(filename));
}

TEST_F(IoTest, isFileAndIsDirectory) {
    EXPECT_FALSE(cb::io::isFile("."));
    EXPECT_TRUE(cb::io::isDirectory("."));
    auto filename = cb::io::mktemp("plainfile");
    EXPECT_TRUE(cb::io::isFile(filename));
    EXPECT_FALSE(cb::io::isDirectory(filename));
    cb::io::rmrf(filename);
}

// Verify cb::io methods support long paths on Windows.
TEST_F(IoTest, longpaths) {
    std::filesystem::path testRootDir("longpaths");
    std::filesystem::path dirPath = testRootDir;
    for (int i = 0; i < 4; i++) {
        dirPath = dirPath / std::string(100, 'a');
    }

    // Clean up any previous test runs.
    if (cb::io::isDirectory(testRootDir.string())) {
        cb::io::rmrf(testRootDir.string());
    }

    // mkdirp
    cb::io::mkdirp(dirPath.string());

    // mkdtemp
    auto tempDir = cb::io::mkdtemp(dirPath.string());

    // isDirectory
    EXPECT_TRUE(cb::io::isDirectory(dirPath.string()));
    EXPECT_TRUE(cb::io::isDirectory(tempDir));

    // Create two files for testing purposes.
    auto const filePath1 =
            cb::io::mktemp((dirPath / "file1").make_preferred().string());
    auto const filePath2 =
            cb::io::mktemp((dirPath / "file2").make_preferred().string());

    // isFile
    EXPECT_TRUE(cb::io::isFile(filePath1));
    EXPECT_TRUE(cb::io::isFile(filePath2));

    // findFilesWithPrefix
    auto files = cb::io::findFilesWithPrefix(dirPath, "file");
    EXPECT_EQ(2, files.size());
    EXPECT_NE(files.end(), std::find(files.begin(), files.end(), filePath1));
    EXPECT_NE(files.end(), std::find(files.begin(), files.end(), filePath2));
    files = cb::io::findFilesWithPrefix(dirPath, "foo");
    EXPECT_EQ(0, files.size());

    // findFilesContaining
    files = cb::io::findFilesContaining(dirPath, "file");
    EXPECT_EQ(2, files.size());
    EXPECT_NE(files.end(), std::find(files.begin(), files.end(), filePath1));
    EXPECT_NE(files.end(), std::find(files.begin(), files.end(), filePath2));
    files = cb::io::findFilesContaining(dirPath, "foo");
    EXPECT_EQ(0, files.size());

    // rmrf
    cb::io::rmrf(testRootDir.string());
    EXPECT_FALSE(cb::io::isFile(filePath1));
    EXPECT_FALSE(cb::io::isFile(filePath2));
    EXPECT_FALSE(cb::io::isDirectory(testRootDir.string()));
}

TEST_F(IoTest, mkdirp) {
    std::string path{"/it/would/suck/if/I/could/create/this"};

#ifdef WIN32
    // Try to use an invalid drive on Windows
    bool found = false;
    for (int ii = 'D'; ii < 'Z'; ++ii) {
        std::string drive;
        drive.push_back((char)ii);
        drive.push_back(':');
        if (!cb::io::isDirectory(drive)) {
            found = true;
            path = drive + path;
            break;
        }
    }
    if (!found) {
        FAIL() << "Failed to locate an unused drive";
    }
#endif
    EXPECT_THROW(cb::io::mkdirp(path), std::runtime_error);

    cb::io::mkdirp(".");
    cb::io::mkdirp("/");
    cb::io::mkdirp("foo/bar");
    EXPECT_TRUE(cb::io::isDirectory("foo/bar"));
    cb::io::rmrf("foo");
    EXPECT_FALSE(cb::io::isDirectory("foo/bar"));
    EXPECT_FALSE(cb::io::isDirectory("foo"));
}

TEST_F(IoTest, maximizeFileDescriptors) {
    auto limit = cb::io::maximizeFileDescriptors(32);
    EXPECT_LE(32u, limit) << "FAIL: I should be able to set it to at least 32";

    limit = cb::io::maximizeFileDescriptors(
        std::numeric_limits<uint32_t>::max());
    if (limit != std::numeric_limits<uint32_t>::max()) {
        // windows don't have a max limit, and that could be for other platforms
        // as well..
        EXPECT_EQ(limit, cb::io::maximizeFileDescriptors(limit + 1))
             << "FAIL: I expected maximizeFileDescriptors to return "
                      << "the same max limit two times in a row";
    }

    limit = cb::io::maximizeFileDescriptors(
        std::numeric_limits<uint64_t>::max());
    if (limit != std::numeric_limits<uint64_t>::max()) {
        // windows don't have a max limit, and that could be for other platforms
        // as well..
        EXPECT_EQ(limit, cb::io::maximizeFileDescriptors(limit + 1))
                    << "FAIL: I expected maximizeFileDescriptors to return "
                    << "the same max limit two times in a row";
    }
}

TEST_F(IoTest, loadFile) {
    auto filename = cb::io::mktemp("loadfile_test");
    ASSERT_FALSE(filename.empty())
            << "FAIL: Expected to create tempfile without mask";

    const std::string content{"Hello world!!!"};
    auto* fp = fopen(filename.c_str(), "wb+");
    fprintf(fp, "%s", content.c_str());
    fclose(fp);
    EXPECT_EQ(content, cb::io::loadFile(filename));
    cb::io::rmrf(filename);
}

TEST_F(IoTest, loadFileWithLimit) {
    auto filename = cb::io::mktemp("loadfile_test");
    ASSERT_FALSE(filename.empty())
            << "FAIL: Expected to create tempfile without mask";

    std::string content{"Hello world!!!"};
    auto* fp = fopen(filename.c_str(), "wb+");
    fprintf(fp, "%s", content.c_str());
    fclose(fp);
    content.resize(content.size() / 2);
    EXPECT_EQ(content, cb::io::loadFile(filename, {}, content.size()));
    cb::io::rmrf(filename);
}

TEST_F(IoTest, tokenizeFileLineByLine) {
    auto filename = cb::io::mktemp("tokenize_file_test");
    auto* fp = fopen(filename.c_str(), "wb+");
    fprintf(fp, "This is the first line\r\nThis is the second line\n");
    fclose(fp);

    int count = 0;
    cb::io::tokenizeFileLineByLine(
            filename,
            [&count](const std::vector<std::string_view>& tokens) -> bool {
                ++count;
                EXPECT_LE(count, 3) << "There is only two lines in the file";
                EXPECT_EQ(5, tokens.size());
                EXPECT_EQ("This", tokens[0]);
                EXPECT_EQ("is", tokens[1]);
                EXPECT_EQ("the", tokens[2]);
                if (count == 1) {
                    EXPECT_EQ("first", tokens[3]);
                } else {
                    EXPECT_EQ("second", tokens[3]);
                }
                EXPECT_EQ("line", tokens[4]);
                return true;
            });

    // Verify that we stop parsing when we told it to
    count = 0;
    cb::io::tokenizeFileLineByLine(
            filename, [&count](const std::vector<std::string_view>&) -> bool {
                ++count;
                EXPECT_EQ(1, count);
                return false;
            });

    cb::io::rmrf(filename);
}

TEST_F(IoTest, DirectoryIteratorPermissionsViolation) {
    std::filesystem::path testRootDir("directoryPermissionsTest");
    std::filesystem::path dirPath = testRootDir;

    // Clean up any previous test runs
    if (cb::io::isDirectory(testRootDir.string())) {
        cb::io::rmrf(testRootDir.string());
    }

    // Create the test directory and ensure that it exists
    cb::io::mkdirp(dirPath.string());
    ASSERT_TRUE(cb::io::isDirectory(dirPath.string()));

    // Create two files
    auto const filePath1 =
            cb::io::mktemp((dirPath / "file1").make_preferred().string());
    auto const filePath2 =
            cb::io::mktemp((dirPath / "file2").make_preferred().string());

    // Ensure that the files have been created
    ASSERT_TRUE(cb::io::isFile(filePath1));
    ASSERT_TRUE(cb::io::isFile(filePath2));

    std::error_code ec;
    // Remove read permissions on the temporary directory
    auto targetPermissions = std::filesystem::perms::owner_read |
                             std::filesystem::perms::group_read |
                             std::filesystem::perms::others_read;

    std::filesystem::permissions(dirPath,
                                 targetPermissions,
                                 std::filesystem::perm_options::remove,
                                 ec);
    ASSERT_FALSE(ec) << "Unable to remove read permissions on test directory";
    ec.clear();

    std::vector<std::string> files;

    // Prevent these testcases from running on windows as windows doesn't have
    // POSIX-style permissions for directories.
#ifndef WIN32
    // Ensure that findFilesWithPrefix fails
    files = cb::io::findFilesWithPrefix(dirPath, "file", ec);
    ASSERT_TRUE(ec) << "Expected findFilesWithPrefix to fail when I don't have "
                       "read permissions on the directory";
    ASSERT_EQ(0, files.size());
    ec.clear();

    // Ensure that findFilesContaining fails
    files = cb::io::findFilesContaining(dirPath, "file", ec);
    ASSERT_TRUE(ec) << "Expected findFilesContaining to fail when I don't have "
                       "read permissions on the directory";
    ASSERT_EQ(0, files.size());
    ec.clear();
#endif

    // Ensure that both fail on a non-existent directory
    files = cb::io::findFilesContaining("does_not_exist", "file", ec);
    ASSERT_TRUE(ec) << "Expected findFilesContaining to fail when I attempt to "
                       "read a non-existent directory";
    ec.clear();
    files = cb::io::findFilesWithPrefix("does_not_exist", "file", ec);
    ASSERT_TRUE(ec) << "Expected findFilesWithPrefix to fail when I attempt to "
                       "read a non-existent directory";
    ASSERT_EQ(0, files.size());
    ec.clear();

    // restore read permissions on the temporary directory
    std::filesystem::permissions(
            dirPath, targetPermissions, std::filesystem::perm_options::add, ec);
    ASSERT_FALSE(ec) << "Unable to add read permissions on test directory";

    // Clean up all the directories we created for the tests.
    cb::io::rmrf(testRootDir.string());
    ASSERT_FALSE(cb::io::isFile(filePath1));
    ASSERT_FALSE(cb::io::isFile(filePath2));
    ASSERT_FALSE(cb::io::isDirectory(testRootDir.string()));
}

#ifdef WIN32
TEST_F(IoTest, sanitizePath) {
    const std::string content{"/hello/world\\foo"};
    std::filesystem::path path(content);
    EXPECT_EQ("\\hello\\world\\foo", path.make_preferred().string());
}
#endif

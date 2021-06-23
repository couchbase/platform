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

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <platform/dirutils.h>
#include <string>
#include <vector>
#include <system_error>

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(a, b) _mkdir(a)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

static bool CreateDirectory(const std::string& dir) {
    if (mkdir(dir.c_str(), S_IXUSR | S_IWUSR | S_IRUSR) != 0) {
        if (errno != EEXIST) {
            return false;
        }
    }
    return true;
}

static bool exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}


class IoTest : public ::testing::Test {
public:

    static void SetUpTestCase();

    static void TearDownTestCase();

protected:

    // A small filesystem we can play around with
    static const std::vector<std::string> vfs;
};

const std::vector<std::string> IoTest::vfs = {
    {"fs"},
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
    {"fs/d1/d1"}
};

void IoTest::SetUpTestCase() {
    for (const auto& file : vfs) {
        ASSERT_TRUE(CreateDirectory(file)) << "failed to create directory";
        ASSERT_TRUE(exists(file)) << "directory " << file << " does not exists";
    }
}

void IoTest::TearDownTestCase() {
    // Expecting the rmrf to work ;)
    cb::io::rmrf("fs");
}

TEST_F(IoTest, dirname) {
    // Check the simple case
    EXPECT_EQ("foo", cb::io::dirname("foo\\bar"));
    EXPECT_EQ("foo", cb::io::dirname("foo/bar"));

    // Make sure that we remove double an empty chunk
    EXPECT_EQ("foo", cb::io::dirname("foo\\\\bar"));
    EXPECT_EQ("foo", cb::io::dirname("foo//bar"));

    // Make sure that we handle the case without a directory
    EXPECT_EQ(".", cb::io::dirname("bar"));
    EXPECT_EQ(".", cb::io::dirname(""));

    // Absolute directories
    EXPECT_EQ("\\", cb::io::dirname("\\bar"));
    EXPECT_EQ("\\", cb::io::dirname("\\\\bar"));
    EXPECT_EQ("/", cb::io::dirname("/bar"));
    EXPECT_EQ("/", cb::io::dirname("//bar"));

    // Test that we work with multiple directories
    EXPECT_EQ("1/2/3/4/5", cb::io::dirname("1/2/3/4/5/6"));
    EXPECT_EQ("1\\2\\3\\4\\5", cb::io::dirname("1\\2\\3\\4\\5\\6"));
    EXPECT_EQ("1/2\\4/5", cb::io::dirname("1/2\\4/5\\6"));
}

TEST_F(IoTest, basename) {
    EXPECT_EQ("bar", cb::io::basename("foo\\bar"));
    EXPECT_EQ("bar", cb::io::basename("foo/bar"));
    EXPECT_EQ("bar", cb::io::basename("foo\\\\bar"));
    EXPECT_EQ("bar", cb::io::basename("foo//bar"));
    EXPECT_EQ("bar", cb::io::basename("bar"));
    EXPECT_EQ("", cb::io::basename(""));
    EXPECT_EQ("bar", cb::io::basename("\\bar"));
    EXPECT_EQ("bar", cb::io::basename("\\\\bar"));
    EXPECT_EQ("bar", cb::io::basename("/bar"));
    EXPECT_EQ("bar", cb::io::basename("//bar"));
    EXPECT_EQ("6", cb::io::basename("1/2/3/4/5/6"));
    EXPECT_EQ("6", cb::io::basename("1\\2\\3\\4\\5\\6"));
    EXPECT_EQ("6", cb::io::basename("1/2\\4/5\\6"));
}

TEST_F(IoTest, findFilesWithPrefix) {
    auto vec = cb::io::findFilesWithPrefix("fs");
    EXPECT_EQ(1u, vec.size());

    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "." PATH_SEPARATOR "fs"));


    vec = cb::io::findFilesWithPrefix("fs", "d");
    EXPECT_EQ(3u, vec.size());

    // We don't know the order of the files in the result..
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "d1"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "d2"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "d3"));

    vec = cb::io::findFilesWithPrefix("fs", "1");
    EXPECT_EQ(1u, vec.size());
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "1"));

    vec = cb::io::findFilesWithPrefix("fs", "");
    EXPECT_EQ(vfs.size() - 2, vec.size());
}

TEST_F(IoTest, findFilesContaining) {
    auto vec = cb::io::findFilesContaining("fs", "");
    EXPECT_EQ(vfs.size() - 2, vec.size());

    vec = cb::io::findFilesContaining("fs", "2");
    EXPECT_EQ(7u, vec.size());
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "d2"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "e2"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "f2c"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "g2"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "2"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "2c"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(),
                                   "fs" PATH_SEPARATOR "2d"));
}

TEST_F(IoTest, mktemp) {
    auto filename = cb::io::mktemp("foo");
    EXPECT_FALSE(filename.empty())
                << "FAIL: Expected to create tempfile without mask";
    EXPECT_TRUE(cb::io::isFile(filename));
    EXPECT_NO_THROW(cb::io::rmrf(filename));
    EXPECT_FALSE(cb::io::isFile(filename));
    EXPECT_FALSE(cb::io::isDirectory(filename));

    filename = cb::io::mktemp("barXXXXXX");
    EXPECT_FALSE(filename.empty())
                << "FAIL: Expected to create tempfile mask";
    EXPECT_TRUE(cb::io::isFile(filename));
    EXPECT_NO_THROW(cb::io::rmrf(filename));
    EXPECT_FALSE(cb::io::isFile(filename));
    EXPECT_FALSE(cb::io::isDirectory(filename));
}

TEST_F(IoTest, isFileAndIsDirectory) {
    EXPECT_FALSE(cb::io::isFile("."));
    EXPECT_TRUE(cb::io::isDirectory("."));
    auto filename = cb::io::mktemp("plainfile");
    EXPECT_TRUE(cb::io::isFile(filename));
    EXPECT_FALSE(cb::io::isDirectory(filename));
    EXPECT_NO_THROW(cb::io::rmrf(filename));
}

TEST_F(IoTest, removeNonExistentFile) {
    EXPECT_THROW(cb::io::rmrf("foo"), std::system_error)
                << "Expected system error for removing non-existent file";
}

TEST_F(IoTest, getcwd) {
    auto cwd = cb::io::getcwd();
    // I can't really determine the correct value here, but it shouldn't be
    // empty ;-)
    ASSERT_FALSE(cwd.empty());
}

#ifdef WIN32
// Verify cb::io methods support long paths on Windows.
TEST_F(IoTest, longpaths) {
    constexpr auto testRootDir = "longpaths";
    std::string dirPath = testRootDir;
    for (int i = 0; i < 4; i++) {
        dirPath += PATH_SEPARATOR + std::string(100, 'a');
    }
    constexpr auto prefix = R"(\\?\)";

    // Clean up any previous test runs.
    if (cb::io::isDirectory(testRootDir)) {
        cb::io::rmrf(testRootDir);
    }

    // mkdirp
    ASSERT_NO_THROW(cb::io::mkdirp(dirPath));

    // mkdtemp
    auto tempDir = cb::io::mkdtemp(dirPath);

    // isDirectory
    EXPECT_TRUE(cb::io::isDirectory(dirPath));
    EXPECT_TRUE(cb::io::isDirectory(tempDir));

    // Create two files for testing purposes.
    auto const filePath1 = cb::io::mktemp(dirPath + PATH_SEPARATOR + "file1");
    auto const filePath2 = cb::io::mktemp(dirPath + PATH_SEPARATOR + "file2");

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
    ASSERT_NO_THROW(cb::io::rmrf(testRootDir));
    EXPECT_FALSE(cb::io::isFile(filePath1));
    EXPECT_FALSE(cb::io::isFile(filePath2));
    EXPECT_FALSE(cb::io::isDirectory(testRootDir));
}
#endif

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

    EXPECT_NO_THROW(cb::io::mkdirp("."));
    EXPECT_NO_THROW(cb::io::mkdirp("/"));
    EXPECT_NO_THROW(cb::io::mkdirp("foo/bar"));
    EXPECT_NO_THROW(cb::io::isDirectory("foo/bar"));
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

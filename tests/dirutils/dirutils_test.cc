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

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <platform/dirutils.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
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

TEST_F(IoTest, mkdirp) {
#ifndef WIN32
    EXPECT_THROW(cb::io::mkdirp("/it/would/suck/if/I/could/create/this"),
                 std::runtime_error);
#endif
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

TEST(LoadLibrary, NoLibrary) {
    try {
        cb::io::loadLibrary({});
        FAIL() << "An exception should be thrown";
    } catch (const std::exception& exp) {
        EXPECT_STREQ("LibraryHandleImpl: shared object name cannot be empty",
                     exp.what());
    }
}

TEST(LoadLibrary, UnknownLibrary) {
    try {
        cb::io::loadLibrary("itwouldsuckifthisexists");
        FAIL() << "An exception should be thrown";
    } catch (const std::exception& exp) {
    }
}

TEST(LoadLibrary, ValidTestLibrary) {
    auto lib = cb::io::loadLibrary("./platform-dirutils-test-library");

#ifdef WIN32
    EXPECT_EQ(".\\platform-dirutils-test-library", lib->getName());
#else
    EXPECT_EQ("./platform-dirutils-test-library.so", lib->getName());
#endif
}

TEST(LoadLibrary, ValidTestLibraryWithSoExtension) {
    auto lib = cb::io::loadLibrary("./platform-dirutils-test-library.so");

#ifdef WIN32
    EXPECT_EQ(".\\platform-dirutils-test-library.dll", lib->getName());
#else
    EXPECT_EQ("./platform-dirutils-test-library.so", lib->getName());
#endif
}

TEST(LoadLibrary, FindSymbol) {
    auto lib = cb::io::loadLibrary("./platform-dirutils-test-library.so");

    // Check that we can lookup a value
    const auto* valueptr = lib->find<const int*>("value");
    EXPECT_EQ(5, *valueptr);

    // Lookup the getter
    auto getValue = lib->find<int (*)()>("getValue");
    EXPECT_EQ(5, getValue());

    // And the setter
    auto setValue = lib->find<void (*)(int)>("setValue");
    setValue(100);
    EXPECT_EQ(100, *valueptr);
    EXPECT_EQ(100, getValue());
}

TEST(LoadLibrary, Unload) {
    // Unload and load the library and verify that when we reload it the
    // static variables get reset
    auto lib = cb::io::loadLibrary("./platform-dirutils-test-library.so");
    const auto* valueptr = lib->find<const int*>("value");
    EXPECT_EQ(5, *valueptr);
    *const_cast<int*>(valueptr) = 100;
    EXPECT_EQ(100, *valueptr);
    lib.reset();

    lib = cb::io::loadLibrary("./platform-dirutils-test-library.so");
    valueptr = lib->find<const int*>("value");
    EXPECT_EQ(5, *valueptr);
}

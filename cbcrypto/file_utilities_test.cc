/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "cbcrypto/file_utilities.h"

#include <cbcrypto/common.h>
#include <cbcrypto/file_reader.h>
#include <cbcrypto/file_writer.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>
#include <filesystem>

using namespace cb::crypto;
using namespace std::string_view_literals;
using namespace std::string_literals;

class FileUtilitiesTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir = cb::io::mkdtemp("FileUtilitiesTest");
    }

    void TearDown() override {
        remove_all(dir);
    }

    void create_file(const std::string& name, const std::string& content) {
        std::shared_ptr<DataEncryptionKey> key = DataEncryptionKey::generate();
        auto writer = FileWriter::create(key, dir / name);
        writer->write(content);
        writer->flush();
        writer.reset();

        files[name] = key->getId();
    }

    std::filesystem::path dir;

    std::unordered_map<std::string, std::string> files;
};

TEST_F(FileUtilitiesTest, findDeksInUse) {
    for (const auto& name : {"file1.cef", "file2.cef", "file3.cef"}) {
        create_file(name, "This is the content");
    }

    /// verify the files are there and the keys are different
    ASSERT_EQ(3, files.size());
    EXPECT_NE(files["file1.cef"], files["file2.cef"]);
    EXPECT_NE(files["file1.cef"], files["file3.cef"]);
    EXPECT_NE(files["file3.cef"], files["file2.cef"]);

    auto keys = findDeksInUse(
            dir,
            [](const std::filesystem::path& p) {
                return p.extension().string() == ".cef";
            },
            [](std::string_view, const nlohmann::json&) {});
    EXPECT_EQ(3, keys.size());
    for (const auto& [file, key] : files) {
        EXPECT_EQ(1, keys.count(key)) << key << " " << file;
    }

    // Search for just a limited subset of the files
    keys = findDeksInUse(
            dir,
            [](const std::filesystem::path& p) {
                return p.filename().string() == "file1.cef";
            },
            [](std::string_view, const nlohmann::json&) {});
    EXPECT_EQ(1, keys.size());
    EXPECT_EQ(1, keys.count(files["file1.cef"]));

    // Ensure that we call the error callback for a non-existing directory (
    // or other errors from traversing the directory)
    bool error = false;
    keys = findDeksInUse(
            dir / "no-such-directory",
            [](const std::filesystem::path& p) { return true; },
            [&error](auto message, const auto& json) {
                EXPECT_EQ("Error occurred while traversing directory", message);
                EXPECT_TRUE(json.contains("error")) << json.dump();
                error = true;
            });
    EXPECT_TRUE(error) << "Error callback not called";
    EXPECT_TRUE(keys.empty());
}

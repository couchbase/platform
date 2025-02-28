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
#include "cbcrypto/key_store.h"

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
        keystore = {};
        keystore.setActiveKey(DataEncryptionKey::generate());
        dir = cb::io::mkdtemp("FileUtilitiesTest");
    }

    void TearDown() override {
        remove_all(dir);
    }

    void create_file(const std::string& name,
                     std::string_view content,
                     const bool encrypted = true,
                     const std::size_t max_chunk_size =
                             std::numeric_limits<std::size_t>::max()) {
        std::shared_ptr<DataEncryptionKey> key;
        if (encrypted) {
            key = DataEncryptionKey::generate();
            keystore.add(key);
        }
        auto writer = FileWriter::create(key, dir / name);
        while (!content.empty()) {
            auto chunk =
                    content.substr(0, std::min(content.size(), max_chunk_size));
            content.remove_prefix(chunk.size());
            writer->write(chunk);
        }
        writer->flush();
        writer.reset();

        files[name] = encrypted ? key->getId() : "";
    }

    std::filesystem::path dir;

    std::unordered_map<std::string, std::string> files;
    KeyStore keystore;
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
            [](const std::filesystem::path&) { return true; },
            [&error](auto message, const auto& json) {
                EXPECT_EQ("Error occurred while traversing directory", message);
                EXPECT_TRUE(json.contains("error")) << json.dump();
                error = true;
            });
    EXPECT_TRUE(error) << "Error callback not called";
    EXPECT_TRUE(keys.empty());
}

TEST_F(FileUtilitiesTest, rewriteUnencryptedToEncrypted) {
    create_file("file.txt", "This is the content", false);

    maybeRewriteFiles(
            dir,
            [](const auto& path, auto) {
                return path.filename().string() == "file.txt";
            },
            keystore.getActiveKey(),
            [this](auto id) { return keystore.lookup(id); },
            [](std::string_view, const nlohmann::json&) {});

    auto file = dir / "file.txt";
    EXPECT_FALSE(std::filesystem::exists(file));
    file = dir / "file.cef";
    EXPECT_TRUE(std::filesystem::exists(file));

    const auto reader = FileReader::create(
            file, [this](auto id) { return keystore.lookup(id); });
    EXPECT_TRUE(reader->is_encrypted());
    EXPECT_EQ("This is the content", reader->read());
}

TEST_F(FileUtilitiesTest, rewriteEncryptedToUnencrypted) {
    create_file("file.cef", "This is the content");
    maybeRewriteFiles(
            dir,
            [](const auto& path, auto) {
                return path.filename().string() == "file.cef";
            },
            {},
            [this](auto id) { return keystore.lookup(id); },
            [](std::string_view, const nlohmann::json&) {});

    auto file = dir / "file.cef";
    EXPECT_FALSE(std::filesystem::exists(file));
    file = dir / "file.txt";
    EXPECT_TRUE(std::filesystem::exists(file));

    const auto reader = FileReader::create(
            file, [this](auto id) { return keystore.lookup(id); });
    EXPECT_FALSE(reader->is_encrypted());
    EXPECT_EQ("This is the content", reader->read());
}

TEST_F(FileUtilitiesTest, rewriteUsingCertainKey) {
    create_file("file1.cef", "This is the content");
    const auto key_id = files["file1.cef"];

    maybeRewriteFiles(
            dir,
            [&key_id](const auto&, auto id) { return key_id == id; },
            keystore.getActiveKey(),
            [this](auto id) { return keystore.lookup(id); },
            [](std::string_view, const nlohmann::json&) {});

    std::string requested;
    const auto reader =
            FileReader::create(dir / "file1.cef", [this, &requested](auto id) {
                requested = id;
                return keystore.lookup(id);
            });

    EXPECT_TRUE(reader->is_encrypted());
    EXPECT_EQ("This is the content", reader->read());
    EXPECT_EQ(requested, keystore.getActiveKey()->getId());
}

TEST_F(FileUtilitiesTest, rewriteTruncatedFile) {
    std::string data(1024 * 1024, 'a');
    create_file("file1.cef", data, true, 1024);

    const auto filename = dir / "file1.cef";
    const auto size = file_size(filename);
    // truncate the file with a partial final block
    resize_file(filename, size - 512);

    maybeRewriteFiles(
            dir,
            [](const auto& path, auto) {
                return path.filename() == "file1.cef";
            },
            keystore.getActiveKey(),
            [this](auto id) { return keystore.lookup(id); },
            [](std::string_view, const nlohmann::json&) {});

    std::string requested;
    const auto reader =
            FileReader::create(filename, [this, &requested](auto id) {
                requested = id;
                return keystore.lookup(id);
            });

    EXPECT_TRUE(reader->is_encrypted());
    const auto content = reader->read();
    EXPECT_EQ(content.size(), data.size() - 1024);
    data.resize(data.size() - 1024);
    EXPECT_EQ(data, content);
    EXPECT_EQ(requested, keystore.getActiveKey()->getId());
}
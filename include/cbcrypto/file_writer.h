/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once
#include <cbcrypto/common.h>
#include <cbcrypto/encrypted_file_header.h>
#include <filesystem>
#include <string_view>

namespace cb::crypto {

/**
 * The FileWriter class allows for writing to a file with or without
 * encryption (depending on if an encryption key is present). The primary
 * motivation for the class is to simplify the logic in the other classes
 * which wants to write both encrypted and plain versions of the files
 * depending on the configuration.
 */
class FileWriter {
public:
    /**
     * Create a new instance of the FileWriter
     *
     * @param kdk The key to use to write the files. If no key is present
     *            the data is written unencrypted
     * @param path The name of the file to write
     * @param buffer_size An optional buffer size to buffer data before
     *                    encrypting and writing to disk - useful for encrypted
     *                    logfiles to avoid writing small chunks
     * @return A new FileWriter instance
     */
    static std::unique_ptr<FileWriter> create(
            const SharedKeyDerivationKey& kdk,
            std::filesystem::path path,
            size_t buffer_size = 0,
            Compression compression = Compression::None);

    /**
     * Create a new instance of the FileWriter which encrypts data and wraps an
     * existing writer
     *
     * @param kdk The key to use to write the file
     * @param writer FileWriter to wrap with encryption
     * @param buffer_size An optional buffer size to buffer data before
     *                    encrypting and writing to disk - useful for encrypted
     *                    logfiles to avoid writing small chunks
     * @return A new FileWriter instance
     */
    static std::unique_ptr<FileWriter> wrap_with_encryption(
            const SharedKeyDerivationKey& kdk,
            std::unique_ptr<FileWriter> writer,
            size_t buffer_size = 0,
            Compression compression = Compression::None);

    /// Is the file being read encrypted or not
    [[nodiscard]] virtual bool is_encrypted() const = 0;

    /// Return the current size of the file (in bytes)
    [[nodiscard]] virtual size_t size() const = 0;

    /**
     * Write the provided chunk of data to the file
     * @param chunk The data to write
     * @throws std::runtime_error if an error occurs
     */
    virtual void write(std::string_view chunk) = 0;

    /// Flush data to the file, and throw an exception if an error occurs
    virtual void flush() = 0;

    /// Close the stream. No more data can be written after this call
    virtual void close() = 0;

    virtual ~FileWriter() = default;

protected:
    FileWriter() = default;
};

} // namespace cb::crypto

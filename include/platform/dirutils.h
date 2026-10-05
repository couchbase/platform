/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cb::io {

#ifdef WIN32
const char DirectorySeparator{'\\'};
#else
const char DirectorySeparator{'/'};
#endif

/// Converts given path into extended-length path for Windows. This conversion
/// is required when accessing paths longer than MAX_PATH (260). Note: Using
/// such extended-length paths requires using Unicode version of file APIs (W
/// suffixed, eg CreateFileW).
/// @param path to be converted.
/// @return extended-length path.
/// @see
/// https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
std::filesystem::path makeExtendedLengthPath(const std::string& path);

/**
 * Return the directory part of an absolute path
 */
std::string dirname(const std::string& dir);

/**
 * Return the filename part of an absolute path
 */
std::string basename(const std::string& name);

/**
 * Return a vector containing all of the files starting with a given
 * name stored in a given directory
 */
std::vector<std::string> findFilesWithPrefix(const std::string& dir,
                                             const std::string& name);

/**
 * Return a vector containing all of the files starting with a given
 * name specified with this absolute path
 */
std::vector<std::string> findFilesWithPrefix(const std::string& name);

/**
 * Return a vector containing all of the files containing a given
 * substring located in a given directory
 */
std::vector<std::string> findFilesContaining(const std::string& dir,
                                             const std::string& name);

/**
 * Delete a file or directory (including subdirectories)
 * @param path path of the file or directory that is being removed
 * @throws system_error in case of any errors during deletion
 */
void rmrf(const std::string& path);

/**
 * Check if a directory exists or not
 */
bool isDirectory(const std::string& directory);

/**
 * Check if a path exists and is a file
 */
bool isFile(const std::string& file);

/**
 * Try to create directory including all of the parent directories.
 *
 * Calls sanitizePath() on the given directory before attempting
 * to create it.
 *
 * @param directory the directory to create
 * @throws std::runtime_error if an error occurs
 */
void mkdirp(std::string directory);

/**
 * Create a unique temporary file with the given prefix.
 *
 * This method is implemented by using cb_mktemp, but the caller
 * does not need to add the XXXXXX in the filename.
 *
 * @param prefix The prefix to use in the filename.
 * @return The unique filename
 */
std::string mktemp(const std::string& prefix);

/**
 * Create a unique temporary directory with the given prefix.
 *
 * It works similar to mktemp
 *
 * @param prefix The prefix to use in the directory name.
 * @return The unique directory name
 */
std::string mkdtemp(const std::string& prefix);

/**
 * Try to set the maximum number of file descriptors to the requested
 * limit, and return the number we could get.
 *
 * On a Unix system this limit affects files and sockets
 *
 * @param limit the requested maximum number of file descriptors
 * @return the maximum number of file descriptors
 * @throws std::system_error if we fail to determine the maximum number
 *         of file descriptor
 */
uint64_t maximizeFileDescriptors(uint64_t limit);

/**
 * Windows use '\' as the directory separator character (but internally it
 * allows '/', but I've seen problems where I try to mix '\' and '/' in the
 * same path. This method replace all occurrences of '/' with '\' on windows.
 *
 * @param path the path to sanitize
 * @return sanitized version of the path
 */
std::string sanitizePath(std::string path);

/**
 * Load the named file
 *
 * @param name the name of the file to load
 * @return The content of the file
 * @param waittime The number of microseconds to wait for the file to appear
 *                 if its not there (yet)
 * @param bytesToRead Read upto this many bytes
 * @throws std::system_exception if an error occurs opening / reading the file
 *         std::bad_alloc for memory allocation errors
 */
std::string loadFile(const std::string& name,
                     std::chrono::microseconds waittime = {},
                     size_t bytesToRead = std::numeric_limits<size_t>::max());

/**
 * Save the content to the named file.
 *
 * @param path the name of the file to save
 * @param content the content to save
 * @param mode the mode to use when opening the file (std::ios_base::app
 *             appends to the file, otherwise it is truncated)
 * @throws std::system_error if an error occurs opening / writing the file
 *         (the error code contains the errno from the failing operation)
 */
void saveFile(const std::filesystem::path& path,
              std::string_view content,
              std::ios_base::openmode mode = std::ios_base::trunc |
                                             std::ios_base::binary);

/**
 * Save the content to the named file.
 *
 * @param path the name of the file to save
 * @param content the content to save
 * @param ec Where to store the error code if an error occurs
 * @param mode the mode to use when opening the file
 * @returns true for success, false otherwise and ec contains the error code
 */
[[nodiscard]] bool saveFile(
        const std::filesystem::path& path,
        std::string_view content,
        std::error_code& ec,
        std::ios_base::openmode mode = std::ios_base::trunc |
                                       std::ios_base::binary) noexcept;

/**
 * Flush the named directory to stable storage so that changes to its
 * entries (such as a file created or renamed into it) survive a crash.
 * Windows doesn't support syncing a directory so it is a no-op there.
 *
 * @param directory the directory to sync (empty means current directory)
 * @throws std::system_error if an error occurs opening or syncing
 */
void fsyncDirectory(const std::filesystem::path& directory);

/**
 * Atomically and durably replace the content of the named file.
 *
 * The content is written to a temporary file (path + ".tmp") which is
 * fsync'ed before it is renamed over the destination. Finally the parent
 * directory is fsync'ed (not on Windows) so that the rename itself survives
 * a crash. After a crash the file contains either the old or the new
 * content, never a partially written (or empty) file.
 *
 * @param path the name of the file to save
 * @param content the content to save
 * @throws std::system_error if an error occurs writing, syncing or renaming
 *         the file (the temporary file is removed on failure)
 */
void saveFileAtomic(const std::filesystem::path& path,
                    std::string_view content);

/**
 * Atomically and durably replace the content of the named file.
 *
 * @see saveFileAtomic(const std::filesystem::path&, std::string_view)
 * @param path the name of the file to save
 * @param content the content to save
 * @param ec Where to store the error code if an error occurs
 * @returns true for success, false otherwise and ec contains the error code
 */
[[nodiscard]] bool saveFileAtomic(const std::filesystem::path& path,
                                  std::string_view content,
                                  std::error_code& ec) noexcept;

/**
 * Read a file line by line and tokenize the line with the provided tokens.
 * If the callback returns false parsing of the file will stop.
 *
 * @param name The name of the file to parse
 * @param callback The callback provided by the user
 * @param delim The delimeter used to separate the fields in the file
 * @param allowEmpty Set to true if one wants to allow "empty" slots
 */
void tokenizeFileLineByLine(
        const std::filesystem::path& name,
        std::function<bool(const std::vector<std::string_view>&)> callback,
        char delim = ' ',
        bool allowEmpty = true);

/**
 * Set the file mode to BINARY
 *
 * @param fp the file stream to update
 * @throws std::system_error if an error occurs
 */
void setBinaryMode(FILE* fp);

} // namespace cb::io

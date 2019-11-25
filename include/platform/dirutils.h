/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
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
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <platform/dirutils-visibility.h>

namespace cb {
namespace io {

#ifdef WIN32
const char DirectorySeparator{'\\'};
#else
const char DirectorySeparator{'/'};
#endif

/**
 * Return the directory part of an absolute path
 */
DIRUTILS_PUBLIC_API
std::string dirname(const std::string& dir);

/**
 * Return the filename part of an absolute path
 */
DIRUTILS_PUBLIC_API
std::string basename(const std::string& name);

/**
 * Return a vector containing all of the files starting with a given
 * name stored in a given directory
 */
DIRUTILS_PUBLIC_API
std::vector<std::string> findFilesWithPrefix(const std::string& dir,
                                             const std::string& name);

/**
 * Return a vector containing all of the files starting with a given
 * name specified with this absolute path
 */
DIRUTILS_PUBLIC_API
std::vector<std::string> findFilesWithPrefix(const std::string& name);

/**
 * Return a vector containing all of the files containing a given
 * substring located in a given directory
 */
DIRUTILS_PUBLIC_API
std::vector<std::string> findFilesContaining(const std::string& dir,
                                             const std::string& name);

/**
 * Delete a file or directory (including subdirectories)
 * @param path path of the file or directory that is being removed
 * @throws system_error in case of any errors during deletion
 */
DIRUTILS_PUBLIC_API
void rmrf(const std::string& path);

/**
 * Check if a directory exists or not
 */
DIRUTILS_PUBLIC_API
bool isDirectory(const std::string& directory);

/**
 * Check if a path exists and is a file
 */
DIRUTILS_PUBLIC_API
bool isFile(const std::string& file);

/**
 * Try to create directory including all of the parent directories
 *
 * @param directory the directory to create
 * @throws std::runtime_error if an error occurs
 */
DIRUTILS_PUBLIC_API
void mkdirp(const std::string& directory);

/**
 * Create a unique temporary file with the given prefix.
 *
 * This method is implemented by using cb_mktemp, but the caller
 * does not need to add the XXXXXX in the filename.
 *
 * @param prefix The prefix to use in the filename.
 * @return The unique filename
 */
DIRUTILS_PUBLIC_API
std::string mktemp(const std::string& prefix);

/**
 * Create a unique temporary directory with the given prefix.
 *
 * It works similar to mktemp
 *
 * @param prefix The prefix to use in the directory name.
 * @return The unique directory name
 */
DIRUTILS_PUBLIC_API
std::string mkdtemp(const std::string& prefix);

/**
 * Get the name of the current working directory
 *
 * @return the name of the current working directory
 * @throws std::system_error if we fail to determine the current working
 *         directory
 */
DIRUTILS_PUBLIC_API
std::string getcwd(void);

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
DIRUTILS_PUBLIC_API
uint64_t maximizeFileDescriptors(uint64_t limit);

/**
 * Windows use '\' as the directory separator character (but internally it
 * allows '/', but I've seen problems where I try to mix '\' and '/' in the
 * same path. This method replace all occurrences of '/' with '\' on windows.
 *
 * @param path the path to sanitize
 */
inline void sanitizePath(std::string& path) {
#ifdef WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
}

/**
 * Load the named file
 *
 * @param name the name of the file to load
 * @return The content of the file
 * @throws std::system_exception if an error occurs opening / reading the file
 *         std::bad_alloc for memory allocation errors
 */
DIRUTILS_PUBLIC_API
std::string loadFile(const std::string& name);

/**
 * Set the file mode to BINARY
 *
 * @param fp the file stream to update
 * @throws std::system_error if an error occurs
 */
DIRUTILS_PUBLIC_API
void setBinaryMode(FILE* fp);

} // namespace io
} // namespace cb

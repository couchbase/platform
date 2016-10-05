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

#include <string>
#include <vector>

#include <platform/dirutils-visibility.h>

namespace cb {
namespace io {
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
 */
DIRUTILS_PUBLIC_API
bool rmrf(const std::string& path);

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
 * @return true if success, false otherwise
 */
DIRUTILS_PUBLIC_API
bool mkdirp(const std::string& directory);

/**
 * Create a unique temporary filename with the given prefix.
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
 * Get the name of the current working directory
 *
 * @return the name of the current working directory
 * @throws std::system_error if we fail to determine the current working
 *         directory
 */
DIRUTILS_PUBLIC_API
std::string getcwd(void);
}
}

// For backward source compatibility
namespace CouchbaseDirectoryUtilities = cb::io;

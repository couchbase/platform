/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2012 Couchbase, Inc
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

#ifndef PLATFORM_DIRUTILS_H_
#define PLATFORM_DIRUTILS_H_ 1

#include <platform/visibility.h>

#include <string>
#include <vector>

namespace CouchbaseDirectoryUtilities
{
    /**
     * Return the directory part of an absolute path
     */
    PLATFORM_PUBLIC_API
    std::string dirname(const std::string &dir);

    /**
     * Return the filename part of an absolute path
     */
    PLATFORM_PUBLIC_API
    std::string basename(const std::string &name);

    /**
     * Return a vector containing all of the files starting with a given
     * name stored in a given directory
     */
    PLATFORM_PUBLIC_API
    std::vector<std::string> findFilesWithPrefix(const std::string &dir, const std::string &name);

    /**
     * Return a vector containing all of the files starting with a given
     * name specified with this absolute path
     */
    PLATFORM_PUBLIC_API
    std::vector<std::string> findFilesWithPrefix(const std::string &name);

    /**
     * Return a vector containing all of the files containing a given substring
     * located in a given directory
     */
    PLATFORM_PUBLIC_API
    std::vector<std::string> findFilesContaining(const std::string &dir, const std::string &name);

    /**
     * Delete a file or directory (including subdirectories)
     */
    PLATFORM_PUBLIC_API
    bool rmrf(const std::string &path);


    /**
     * Check if a directory exists or not
     */
    PLATFORM_PUBLIC_API
    bool isDirectory(const std::string &directory);

    /**
     * Try to create directory including all of the parent directories
     *
     * @param directory the directory to create
     * @return true if success, false otherwise
     */
    PLATFORM_PUBLIC_API
    bool mkdirp(const std::string &directory);

}

#endif  // PLATFORM_DIRUTILS_H_

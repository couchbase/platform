/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc
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
#include "config.h"
#include <platform/dirutils.h>

#ifdef _MSC_VER
#include <direct.h>
#define rmdir _rmdir
#define mkdir(a, b) _mkdir(a)
#else
#include <dirent.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <system_error>

namespace CouchbaseDirectoryUtilities
{
    using namespace std;

    static string split(const string &input, bool directory)
    {
        string::size_type path = input.find_last_of("\\/");
        string file;
        string dir;

        if (path == string::npos) {
            dir = ".";
            file = input;
        } else {
            dir = input.substr(0, path);
            if (dir.length() == 0) {
                dir = input.substr(0, 1);
            }

            while (dir.length() > 1 && dir.find_last_of("\\/") == (dir.length() - 1)) {
                if (dir.length() > 1) {
                    dir.resize(dir.length() - 1);
                }
            }

            file = input.substr(path + 1);
        }

        if (directory) {
            return dir;
        } else {
            return file;
        }
    }

    DIRUTILS_PUBLIC_API
    string dirname(const string &dir)
    {
        return split(dir, true);
    }

    DIRUTILS_PUBLIC_API
    string basename(const string &name)
    {
        return split(name, false);
    }

#ifdef _MSC_VER
    DIRUTILS_PUBLIC_API
    vector<string> findFilesWithPrefix(const string &dir, const string &name)
    {
        vector<string> files;
        std::string match = dir + "\\" + name + "*";
        WIN32_FIND_DATA FindFileData;

        HANDLE hFind = FindFirstFileEx(match.c_str(), FindExInfoStandard,
                                       &FindFileData, FindExSearchNameMatch,
                                       NULL, 0);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                string fnm(FindFileData.cFileName);
                if (fnm != "." && fnm != "..") {
                    string entry = dir;
                    entry.append("\\");
                    entry.append(FindFileData.cFileName);
                    files.push_back(entry);
                }
            } while (FindNextFile(hFind, &FindFileData));

            FindClose(hFind);
        }
        return files;
    }
#else
    DIRUTILS_PUBLIC_API
    vector<string> findFilesWithPrefix(const string &dir, const string &name)
    {
        vector<string> files;
        DIR *dp = opendir(dir.c_str());
        if (dp != NULL) {
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                string fnm(de->d_name);
                if (fnm == "." || fnm == "..") {
                    continue;
                }
                if (strncmp(de->d_name, name.c_str(), name.length()) == 0) {
                    string entry = dir;
                    entry.append("/");
                    entry.append(de->d_name);
                    files.push_back(entry);
                }
            }

            closedir(dp);
        }
        return files;
    }
#endif

    DIRUTILS_PUBLIC_API
    vector<string> findFilesWithPrefix(const string &name)
    {
        return findFilesWithPrefix(dirname(name), basename(name));
    }

#ifdef _MSC_VER
    DIRUTILS_PUBLIC_API
    vector<string> findFilesContaining(const string &dir, const string &name)
    {
        vector<string> files;
        std::string match = dir + "\\*" + name + "*";
        WIN32_FIND_DATA FindFileData;

        HANDLE hFind = FindFirstFileEx(match.c_str(), FindExInfoStandard,
                                       &FindFileData, FindExSearchNameMatch,
                                       NULL, 0);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                string fnm(FindFileData.cFileName);
                if (fnm != "." && fnm != "..") {
                    string entry = dir;
                    entry.append("\\");
                    entry.append(FindFileData.cFileName);
                    files.push_back(entry);
                }
            } while (FindNextFile(hFind, &FindFileData));

            FindClose(hFind);
        }
        return files;
    }
#else
    DIRUTILS_PUBLIC_API
    vector<string> findFilesContaining(const string &dir, const string &name)
    {
        vector<string> files;
        DIR *dp = opendir(dir.c_str());
        if (dp != NULL) {
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                if (name.empty() || strstr(de->d_name, name.c_str()) != NULL) {
                    string fnm(de->d_name);
                    if (fnm != "." && fnm != "..") {
                        string entry = dir;
                        entry.append("/");
                        entry.append(de->d_name);
                        files.push_back(entry);
                    }
                }
            }

            closedir(dp);
        }

        return files;
    }
#endif

    DIRUTILS_PUBLIC_API
    bool rmrf(const std::string &path) {
        struct stat st;
        if (stat(path.c_str(), &st) == -1) {
            return false;
        }

        if ((st.st_mode & S_IFDIR) != S_IFDIR) {
            return remove(path.c_str()) == 0;
        }

        if (rmdir(path.c_str()) == 0) {
            return true;
        }

        // Ok, this is a directory. Go ahead and delete it recurively
        std::vector<std::string> directories;
        directories.push_back(path);
        do {
            std::vector<std::string> vec = findFilesContaining(directories.back(), "");
            directories.pop_back();
            std::vector<std::string>::iterator ii;

            for (ii = vec.begin(); ii != vec.end(); ++ii) {
                if (stat(ii->c_str(), &st) == -1) {
                    return false;
                }

                if ((st.st_mode & S_IFDIR) == S_IFDIR) {
                    if (rmdir(ii->c_str()) != 0) {
                        directories.push_back(*ii);
                    }
                } else if (remove(ii->c_str()) != 0) {
                    return false;
                }
            }
        } while (!directories.empty());

        return rmdir(path.c_str()) == 0;
    }

    DIRUTILS_PUBLIC_API
    bool isDirectory(const std::string &directory) {
#ifdef WIN32
        DWORD dwAttrib = GetFileAttributes(directory.c_str());
        if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
            return false;
        }
        return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
#else
        struct stat st;
        if (stat(directory.c_str(), &st) == -1) {
            return false;
        }
        return (S_ISDIR(st.st_mode));
#endif
    }

    DIRUTILS_PUBLIC_API
    bool isFile(const std::string& file) {
#ifdef WIN32
        DWORD dwAttrib = GetFileAttributes(file.c_str());
        if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
            return false;
        }
        return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
        struct stat st;
        if (stat(file.c_str(), &st) == -1) {
            return false;
        }
        return (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode));
#endif
    }

    DIRUTILS_PUBLIC_API
    bool mkdirp(const std::string &directory) {
        struct stat st;
        if (stat(directory.c_str(), &st) == 0) {
            if ((st.st_mode & S_IFDIR) == S_IFDIR) {
                // It exists and is a directory!
                return true;
            }

            // It exists but is a file!
            return false;
        }

        std::string parent = dirname(directory);

        if (!mkdirp(parent)) {
            // failed to create parent directory
            return false;
        }

        if (mkdir(directory.c_str(), S_IREAD | S_IWRITE | S_IEXEC) != 0) {
            return false;
        }

        return true;
    }

    std::string mktemp(const std::string& prefix) {
        static const std::string patternmask{"XXXXXX"};

        std::string pattern = prefix;
        if (pattern.find(patternmask) == pattern.npos) {
            pattern.append(patternmask);
        }
        return std::string {cb_mktemp(const_cast<char*>(pattern.data()))};
    }

    std::string getcwd(void) {
        std::string result(4096, 0);
#ifdef WIN32
        if (GetCurrentDirectory(result.size(), &result[0]) == 0) {
            throw std::system_error(GetLastError(), std::system_category(),
                                    "Failed to determine current working directory");
        }
#else
        if (::getcwd(&result[0], result.size()) == nullptr) {
            throw std::system_error(errno, std::system_category(),
                                    "Failed to determine current working directory");
        }
#endif

        // Trim off any trailing \0 characters.
        result.resize(strlen(result.c_str()));
        return result;
    }
}

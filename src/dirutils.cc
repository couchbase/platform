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
#else

#include <dirent.h>
#include <unistd.h>

#endif

#include <platform/memorymap.h>
#include <platform/strerror.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits>
#include <system_error>

static std::string split(const std::string& input, bool directory) {
    std::string::size_type path = input.find_last_of("\\/");
    std::string file;
    std::string dir;

    if (path == std::string::npos) {
        dir = ".";
        file = input;
    } else {
        dir = input.substr(0, path);
        if (dir.length() == 0) {
            dir = input.substr(0, 1);
        }

        while (dir.length() > 1 &&
               dir.find_last_of("\\/") == (dir.length() - 1)) {
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
std::string cb::io::dirname(const std::string& dir) {
    return split(dir, true);
}

DIRUTILS_PUBLIC_API
std::string cb::io::basename(const std::string& name) {
    return split(name, false);
}

#ifdef _MSC_VER
DIRUTILS_PUBLIC_API
std::vector<std::string> cb::io::findFilesWithPrefix(const std::string &dir,
                                                     const std::string &name)
{
    std::vector<std::string> files;
    std::string match = dir + "\\" + name + "*";
    WIN32_FIND_DATA FindFileData;

    HANDLE hFind = FindFirstFileEx(match.c_str(), FindExInfoStandard,
                                   &FindFileData, FindExSearchNameMatch,
                                   NULL, 0);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string fnm(FindFileData.cFileName);
            if (fnm != "." && fnm != "..") {
                std::string entry = dir;
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
std::vector<std::string> cb::io::findFilesWithPrefix(const std::string& dir,
                                                     const std::string& name) {
    std::vector<std::string> files;
    DIR* dp = opendir(dir.c_str());
    if (dp != NULL) {
        struct dirent* de;
        while ((de = readdir(dp)) != NULL) {
            std::string fnm(de->d_name);
            if (fnm == "." || fnm == "..") {
                continue;
            }
            if (strncmp(de->d_name, name.c_str(), name.length()) == 0) {
                std::string entry = dir;
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
std::vector<std::string> cb::io::findFilesWithPrefix(const std::string& name) {
    return findFilesWithPrefix(dirname(name), basename(name));
}

#ifdef _MSC_VER
DIRUTILS_PUBLIC_API
std::vector<std::string> cb::io::findFilesContaining(const std::string &dir,
                                                            const std::string &name)
{
    std::vector<std::string> files;
    std::string match = dir + "\\*" + name + "*";
    WIN32_FIND_DATA FindFileData;

    HANDLE hFind = FindFirstFileEx(match.c_str(), FindExInfoStandard,
                                   &FindFileData, FindExSearchNameMatch,
                                   NULL, 0);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string fnm(FindFileData.cFileName);
            if (fnm != "." && fnm != "..") {
                std::string entry = dir;
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
std::vector<std::string> cb::io::findFilesContaining(const std::string& dir,
                                                            const std::string& name) {
    std::vector<std::string> files;
    DIR* dp = opendir(dir.c_str());
    if (dp != NULL) {
        struct dirent* de;
        while ((de = readdir(dp)) != NULL) {
            if (name.empty() || strstr(de->d_name, name.c_str()) != NULL) {
                std::string fnm(de->d_name);
                if (fnm != "." && fnm != "..") {
                    std::string entry = dir;
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
void cb::io::rmrf(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) {
        throw std::system_error(errno, std::system_category(),
                                "cb::io::rmrf: stat of " +
                                path + " failed");
    }

    if ((st.st_mode & S_IFDIR) != S_IFDIR) {
        if (remove(path.c_str()) != 0) {
            throw std::system_error(errno, std::system_category(),
                                    "cb::io::rmrf: remove of " +
                                    path + " failed");
        }
        return;
    }

    if (rmdir(path.c_str()) == 0) {
        return;
    }

    // Ok, this is a directory. Go ahead and delete it recursively
    std::vector<std::string> directories;
    std::vector<std::string> emptyDirectories;
    directories.push_back(path);

    // Iterate all the files/directories found in path, when we encounter
    // a sub-directory, that is added to the directories vector so we move
    // deeper into the path.
    do {
        std::vector<std::string> vec =
                findFilesContaining(directories.back(), "");
        emptyDirectories.push_back(directories.back());
        directories.pop_back();
        std::vector<std::string>::iterator ii;

        for (ii = vec.begin(); ii != vec.end(); ++ii) {
            if (stat(ii->c_str(), &st) == -1) {
                throw std::system_error(errno, std::system_category(),
                                        "cb::io::rmrf: stat of file/directory " +
                                        *ii + " under " + path + " failed");
            }

            if ((st.st_mode & S_IFDIR) == S_IFDIR) {
                if (rmdir(ii->c_str()) != 0) {
                    directories.push_back(*ii);
                }
            } else if (remove(ii->c_str()) != 0) {
                throw std::system_error(errno,
                                        std::system_category(),
                                        "cb::io::rmrf: remove of file " + *ii +
                                                " under " + path + " failed");
            }
        }
    } while (!directories.empty());

    // Finally iterate our list of now empty directories, we reverse iterate so
    // that we remove the deepest first
    for (auto itr = emptyDirectories.rbegin(); itr != emptyDirectories.rend();
         ++itr) {
        if (rmdir(itr->c_str()) != 0) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    "cb::io::rmrf: remove of directory " + *itr + " failed");
        }
    }
}

DIRUTILS_PUBLIC_API
bool cb::io::isDirectory(const std::string& directory) {
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
bool cb::io::isFile(const std::string& file) {
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
void cb::io::mkdirp(const std::string& directory) {
    // Bail out immediately if the directory already exists.
    // note that both mkdir and CreateDirectory on windows returns
    // EEXISTS if the directory already exists, BUT it could also
    // return "permission denied" depending on the order the checks
    // is run within "libc"
    if (isDirectory(directory)) {
        return;
    }

#ifdef WIN32
    do {
        if (CreateDirectory(directory.c_str(), nullptr)) {
            return;
        }

        switch (GetLastError()) {
        case ERROR_ALREADY_EXISTS:
            return;
        case ERROR_PATH_NOT_FOUND:
            // one of the paths up the chain does not exists..
            break;
        default:
            throw std::system_error(GetLastError(), std::system_category(),
                                    "cb::io::mkdirp(\"" + directory +
                                    "\") failed");
        }

        // Try to create the parent directory..
        mkdirp(dirname(directory));
    } while (true);
#else
    do {
        if (mkdir(directory.c_str(), S_IREAD | S_IWRITE | S_IEXEC) == 0) {
            return;
        }

        switch (errno) {
        case EEXIST:
            return;
        case ENOENT:
            break;
        default:
            throw std::system_error(errno, std::system_category(),
                                    "cb::io::mkdirp(\"" + directory +
                                    "\") failed");
        }

        // Try to create the parent directory..
        mkdirp(dirname(directory));
    } while (true);
#endif
}

std::string cb::io::mktemp(const std::string& prefix) {
    static const std::string patternmask{"XXXXXX"};
    std::string pattern = prefix;
    if (pattern.find(patternmask) == pattern.npos) {
        pattern.append(patternmask);
    }

    return std::string {cb_mktemp(const_cast<char*>(pattern.data()))};
}

std::string cb::io::getcwd(void) {
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

uint64_t cb::io::maximizeFileDescriptors(uint64_t limit) {
#ifdef WIN32
    return limit;
#else
    struct rlimit rlim;

    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        throw std::system_error(errno, std::system_category(),
                                "getrlimit(RLIMIT_NOFILE, &rlim) failed");
    } else if (limit <= rlim.rlim_cur) {
        return uint64_t(rlim.rlim_cur);
    } else {
        struct rlimit org = rlim;

        rlim.rlim_cur = limit;

        // we don't want to limit the current max if it is higher ;)
        if (rlim.rlim_max < limit) {
            rlim.rlim_max = limit;
        }

        if (setrlimit(RLIMIT_NOFILE, &rlim) == 0) {
            return limit;
        }

        // Ok, we failed to get what we wanted.. Try a binary search
        // to go as high as we can...
        auto min = org.rlim_cur;
        auto max = limit;
        rlim_t last_good = 0;

        while (min <= max) {
            rlim_t avg;

            // make sure we don't overflow
            uint64_t high = std::numeric_limits<uint64_t>::max() - min;
            if (high < max) {
                avg = max / 2;
            } else {
                avg = (min + max) / 2;
            }

            rlim.rlim_cur = rlim.rlim_max = avg;
            if (setrlimit(RLIMIT_NOFILE, &rlim) == 0) {
                last_good = avg;
                min = avg + 1;
            } else {
                max = avg - 1;
            }
        }

        if (last_good == 0) {
            // all setrlimit's failed... lets go fetch it again
            if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
                throw std::system_error(errno, std::system_category(),
                                        "getrlimit(RLIMIT_NOFILE, &rlim) failed");
            }
            return uint64_t(rlim.rlim_cur);
        }

        return uint64_t(last_good);
    }
#endif
}

DIRUTILS_PUBLIC_API
std::string cb::io::loadFile(const std::string& name) {
    cb::MemoryMappedFile map(name.c_str(), cb::MemoryMappedFile::Mode::RDONLY);
    map.open();
    return std::string{reinterpret_cast<char*>(map.getRoot()), map.getSize()};
}

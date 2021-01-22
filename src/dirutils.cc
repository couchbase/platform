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
#include <boost/filesystem.hpp>
#include <platform/dirutils.h>

#include <folly/portability/Dirent.h>
#include <folly/portability/SysResource.h>
#include <folly/portability/SysStat.h>
#include <folly/portability/Unistd.h>

#ifdef _MSC_VER
// Need to define __STDC__ to avoid <io.h> from attempting to redefine
// (with different linkage) functions chmod() and umask() which have already
// been defined by <folly/portability/SysStat.h> above.
#define __STDC__ 1
#include <io.h> // _setmode
#undef __STDC__
#endif

#include <platform/strerror.h>

#include <fcntl.h>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <limits>
#include <system_error>

boost::filesystem::path cb::io::makeExtendedLengthPath(
        const std::string& path) {
    boost::filesystem::path bPath = path;
#ifdef _MSC_VER
    constexpr auto prefix = R"(\\?\)";
    // Prefix exists, return.
    if (path.rfind(prefix, 0) != std::string::npos) {
        return bPath;
    }
    bPath = boost::filesystem::system_complete(bPath);
    bPath = prefix + bPath.string();
#endif
    return bPath;
}

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
std::vector<std::string> cb::io::findFilesWithPrefix(const std::string& dir,
                                                     const std::string& name) {
    std::vector<std::string> files;
    auto longDir = makeExtendedLengthPath(dir);
    auto match = longDir / (name + "*");
    WIN32_FIND_DATAW FindFileData;

    HANDLE hFind = FindFirstFileExW(match.c_str(),
                                    FindExInfoStandard,
                                    &FindFileData,
                                    FindExSearchNameMatch,
                                    NULL,
                                    0);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring fn = FindFileData.cFileName;
            std::string fnm(fn.begin(), fn.end());
            if (fnm != "." && fnm != "..") {
                std::string entry = dir;
                entry.append("\\");
                entry.append(fnm);
                files.push_back(entry);
            }
        } while (FindNextFileW(hFind, &FindFileData));

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
    if (dp != nullptr) {
        struct dirent* de;
        while ((de = readdir(dp)) != nullptr) {
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
    if (dir.empty()) {
        return {};
    }
    std::vector<std::string> files;
    auto longDir = makeExtendedLengthPath(dir);
    auto match = longDir / ("*" + name + "*");
    WIN32_FIND_DATAW FindFileData;

    HANDLE hFind = FindFirstFileExW(match.c_str(),
                                    FindExInfoStandard,
                                    &FindFileData,
                                    FindExSearchNameMatch,
                                    NULL,
                                    0);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring fn(FindFileData.cFileName);
            std::string fnm(fn.begin(), fn.end());
            if (fnm != "." && fnm != "..") {
                std::string entry = dir;
                entry.append("\\");
                entry.append(fnm);
                files.push_back(entry);
            }
        } while (FindNextFileW(hFind, &FindFileData));

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
    if (dp != nullptr) {
        struct dirent* de;
        while ((de = readdir(dp)) != nullptr) {
            if (name.empty() || strstr(de->d_name, name.c_str()) != nullptr) {
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
    auto longPath = makeExtendedLengthPath(path);
    auto longPathStr = longPath.string();
    struct stat st;
    if (stat(longPathStr.c_str(), &st) == -1) {
        throw std::system_error(errno, std::system_category(),
                                "cb::io::rmrf: stat of " +
                                path + " failed");
    }

    if ((st.st_mode & S_IFDIR) != S_IFDIR) {
        if (remove(longPathStr.c_str()) != 0) {
            throw std::system_error(errno, std::system_category(),
                                    "cb::io::rmrf: remove of " +
                                    path + " failed");
        }
        return;
    }

    if (rmdir(longPathStr.c_str()) == 0) {
        return;
    }

    // Ok, this is a directory. Go ahead and delete it recursively
    std::vector<std::string> directories;
    std::vector<std::string> emptyDirectories;
    directories.push_back(longPathStr);

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
            *ii = makeExtendedLengthPath(*ii).string();
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
    auto longDir = makeExtendedLengthPath(directory);
    DWORD dwAttrib = GetFileAttributesW(longDir.c_str());
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
    auto lfile = makeExtendedLengthPath(file);
    DWORD dwAttrib = GetFileAttributesW(lfile.c_str());
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
void cb::io::mkdirp(std::string directory) {
    if (!boost::filesystem::is_directory(directory)) {
        auto longDir = makeExtendedLengthPath(directory);
        boost::filesystem::create_directories(longDir.c_str());
    }
}

std::string cb::io::mktemp(const std::string& prefix) {
    static const std::string patternmask{"XXXXXX"};
    std::string pattern = prefix;

    auto index = pattern.find(patternmask);
    if (index == pattern.npos) {
        index = pattern.size();
        pattern.append(patternmask);
    }

    auto* ptr = const_cast<char*>(pattern.data()) + index;
    auto counter = std::chrono::steady_clock::now().time_since_epoch().count();

    do {
        ++counter;
        sprintf(ptr, "%06" PRIu64, static_cast<uint64_t>(counter) % 1000000);

#ifdef WIN32
        auto longPattern = makeExtendedLengthPath(pattern);
        HANDLE handle = CreateFileW(longPattern.c_str(),
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            return pattern;
        }
#else
        int fd = open(pattern.c_str(),
                      O_WRONLY | O_EXCL | O_CREAT,
                      S_IRUSR | S_IWUSR);
        if (fd != -1) {
            close(fd);
            return pattern;
        }
#endif

    } while (true);
}

std::string cb::io::mkdtemp(const std::string& prefix) {
    static const std::string patternmask{"XXXXXX"};
    std::string pattern = prefix;

    auto index = pattern.find(patternmask);
    char* ptr;
    if (index == pattern.npos) {
        index = pattern.size();
        pattern.append(patternmask);
    }

    ptr = const_cast<char*>(pattern.data()) + index;
    int searching = 1;
    auto counter = std::chrono::steady_clock::now().time_since_epoch().count();

    do {
        ++counter;
        sprintf(ptr, "%06" PRIu64, static_cast<uint64_t>(counter) % 1000000);

#ifdef WIN32
        auto longPattern = makeExtendedLengthPath(pattern);
        if (CreateDirectoryW(longPattern.c_str(), nullptr)) {
            searching = 0;
        }
#else
        if (mkdir(pattern.c_str(), S_IREAD | S_IWRITE | S_IEXEC) == 0) {
            searching = 0;
        }
#endif

    } while (searching);

    return pattern;
}

std::string cb::io::getcwd() {
    return boost::filesystem::current_path().string();
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

void cb::io::setBinaryMode(FILE* fp) {
#ifdef WIN32
    if (_setmode(_fileno(fp), _O_BINARY) == -1) {
        throw std::system_error(
                errno, std::system_category(), "cb::io::setBinaryMode");
    }
#else
    (void)fp;
#endif
}

std::string cb::io::sanitizePath(std::string path) {
#ifdef WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
    return path;
}

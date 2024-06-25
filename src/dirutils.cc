/*
 *     Copyright 2013-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/dirutils.h>
#include <filesystem>

#include <fmt/format.h>
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

std::filesystem::path cb::io::makeExtendedLengthPath(const std::string_view path) {
    std::filesystem::path bPath = path;
#ifdef _MSC_VER
    constexpr auto prefix = R"(\\?\)";
    // Prefix exists, return.
    if (path.rfind(prefix, 0) != std::string_view::npos) {
        return bPath;
    }
    bPath = std::filesystem::absolute(bPath);
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

std::string cb::io::dirname(const std::string& dir) {
    return split(dir, true);
}

std::string cb::io::basename(const std::string& name) {
    return split(name, false);
}

#ifdef _MSC_VER
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

std::vector<std::string> cb::io::findFilesWithPrefix(const std::string& name) {
    return findFilesWithPrefix(dirname(name), basename(name));
}

#ifdef _MSC_VER
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

void cb::io::rmrf(const std::string& path) {
    remove_all(makeExtendedLengthPath(path));
}

bool cb::io::isDirectory(const std::string_view directory) {
    std::error_code ec;
    return is_directory(makeExtendedLengthPath(directory), ec);
}

bool cb::io::isFile(const std::string_view file) {
    std::error_code ec;
    const auto path = makeExtendedLengthPath(file);
    return is_regular_file(path, ec) || is_symlink(path, ec);
}

void cb::io::mkdirp(std::string_view directory) {
    if (!std::filesystem::is_directory(directory)) {
        auto longDir = makeExtendedLengthPath(directory);
        std::filesystem::create_directories(longDir.c_str());
    }
}

std::string cb::io::mktemp(const std::string_view prefix) {
    static const std::string patternmask{"XXXXXX"};
    std::string pattern{prefix};

    auto index = pattern.find(patternmask);
    if (index == std::string::npos) {
        index = pattern.size();
        pattern.append(patternmask);
    }

    auto* ptr = pattern.data() + index;
    auto counter = std::chrono::steady_clock::now().time_since_epoch().count();

    do {
        ++counter;
        fmt::format_to(ptr, "{:06}", counter % 1000000);

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

std::string cb::io::mkdtemp(const std::string_view prefix) {
    static const std::string patternmask{"XXXXXX"};
    std::string pattern {prefix};

    auto index = pattern.find(patternmask);
    if (index == std::string::npos) {
        index = pattern.size();
        pattern.append(patternmask);
    }

    auto* ptr = pattern.data() + index;
    auto counter = std::chrono::steady_clock::now().time_since_epoch().count();

    do {
        ++counter;
        fmt::format_to(ptr, "{:06}", counter % 1000000);
        std::error_code ec;
        if (create_directory(makeExtendedLengthPath(pattern), ec)) {
            return pattern;
        }

    } while (true);
}

uint64_t cb::io::maximizeFileDescriptors(uint64_t limit) {
#ifdef WIN32
    return limit;
#else
    rlimit rlim{};

    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        throw std::system_error(errno,
                                std::system_category(),
                                "getrlimit(RLIMIT_NOFILE, &rlim) failed");
    }

    if (limit <= rlim.rlim_cur) {
        return uint64_t(rlim.rlim_cur);
    }

    rlimit org = rlim;
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
        // Equivalent to std::midpoint from C++20.
        rlim_t avg = min + ((max - min) / 2);

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
            throw std::system_error(errno,
                                    std::system_category(),
                                    "getrlimit(RLIMIT_NOFILE, &rlim) failed");
        }
        return static_cast<uint64_t>(rlim.rlim_cur);
    }

    return static_cast<uint64_t>(last_good);
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

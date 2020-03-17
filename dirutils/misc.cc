/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2020 Couchbase, Inc
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
#include <platform/dirutils.h>

#ifdef _MSC_VER
#include <fcntl.h> // _O_BINARY
#include <folly/portability/Windows.h>
#include <io.h> // _setmode
#else
#include <sys/resource.h>
#endif

#include <folly/portability/Stdlib.h>
#include <folly/portability/Unistd.h>
#include <cinttypes>
#include <cstdio>
#include <cstring>
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

static std::string _mkf(std::string pattern, bool file) {
    static const std::string patternmask{"XXXXXX"};
    if (pattern.find(patternmask) == pattern.npos) {
        pattern.append(patternmask);
    }
    if (file) {
        auto fd = ::mkstemp(pattern.data());
        if (fd == -1) {
            throw std::system_error(errno,
                                    std::system_category(),
                                    "cb::io::mkstemp(" + pattern + ")");
        } else {
            close(fd);
        }
    } else {
        if (::mkdtemp(pattern.data()) == nullptr) {
            throw std::system_error(errno,
                                    std::system_category(),
                                    "cb::io::mkdtemp(" + pattern + ")");
        }
    }
    return pattern;
}

std::string cb::io::mktemp(const std::string& prefix) {
    return _mkf(prefix, true);
}

std::string cb::io::mkdtemp(const std::string& prefix) {
    return _mkf(prefix, false);
}

std::string cb::io::getcwd(void) {
    std::string result(4096, 0);
    if (::getcwd(&result[0], result.size()) == nullptr) {
        throw std::system_error(
                errno,
                std::system_category(),
                "Failed to determine current working directory");
    }

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
        throw std::system_error(errno,
                                std::system_category(),
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
                throw std::system_error(
                        errno,
                        std::system_category(),
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

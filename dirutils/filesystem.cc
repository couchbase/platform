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

#include <platform/strerror.h>
#include <system_error>

// Once we upgrade linux and mac to a C++17 capable C++ compiler we can
// uncomment the following and stop using boost::filesystem
// #include <filesystem>
// namespace fs = std::filesystem;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

std::vector<std::string> cb::io::findFilesWithPrefix(const std::string& dir,
                                                     const std::string& name) {
    if (!isDirectory(dir)) {
        return {};
    }
    std::vector<std::string> files;
    for (auto& p : fs::directory_iterator(dir)) {
        if (name.empty() || p.path().filename().string().find(name) == 0) {
            files.emplace_back(p.path().string());
        }
    }

    return files;
}

std::vector<std::string> cb::io::findFilesWithPrefix(const std::string& name) {
    return findFilesWithPrefix(dirname(name), basename(name));
}

std::vector<std::string> cb::io::findFilesContaining(const std::string& dir,
                                                     const std::string& name) {
    if (!isDirectory(dir)) {
        return {};
    }
    std::vector<std::string> files;
    for (auto& p : fs::directory_iterator(dir)) {
        if (name.empty() ||
            p.path().filename().string().find(name) != std::string::npos) {
            files.emplace_back(p.path().string());
        }
    }

    return files;
}

void cb::io::rmrf(const std::string& path) {
    if (!fs::exists(path)) {
        throw std::system_error(int(std::errc::no_such_file_or_directory),
                                std::system_category(),
                                "cb::io::rmrf");
    }
    fs::remove_all(path);
}

bool cb::io::isDirectory(const std::string& directory) {
    try {
        return fs::is_directory(directory);
    } catch (const fs::filesystem_error& e) {
        // We've got a unit test trying to locate an unknown filesystem
        // on windows (D:), but fs::is_directory throws an exception
        // in those cases..
        switch (e.code().value()) {
        case int(std::errc::no_such_device):
        case int(std::errc::no_such_device_or_address):
        case int(std::errc::device_or_resource_busy):
#ifdef WIN32
        case ERROR_NOT_READY:
#endif
            return false;

        default:
            throw std::system_error(
                    e.code().value(),
                    e.code().category(),
                    std::string("fs::is_directory(\"") + directory +
                            "\"): " + std::to_string(e.code().value()));
        }
    }
}

bool cb::io::isFile(const std::string& file) {
    return (fs::is_regular_file(file) || fs::is_symlink(file));
}

void cb::io::mkdirp(const std::string& directory) {
    // Bail out immediately if the directory already exists.
    // note that both mkdir and CreateDirectory on windows returns
    // EEXISTS if the directory already exists, BUT it could also
    // return "permission denied" depending on the order the checks
    // is run within "libc"
    if (isDirectory(directory)) {
        return;
    }

    fs::create_directories(directory);
}

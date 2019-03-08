/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc.
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

#include <platform/memorymap.h>
#include <stdexcept>
#include <system_error>

#ifdef WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace cb {
namespace io {

#ifdef WIN32

MemoryMappedFile::MemoryMappedFile(const std::string& filename,
                                   const Mode& mode) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &fad) ==
        0) {
        throw std::system_error(
                GetLastError(),
                std::system_category(),
                "cb::io::MemoryMappedFile() GetFileAttributesEx(" + filename +
                        ") failed");
    }
    LARGE_INTEGER sz;
    sz.HighPart = fad.nFileSizeHigh;
    sz.LowPart = fad.nFileSizeLow;
    auto size = (size_t)sz.QuadPart;

    if (size == 0) {
        // Empty file, no need to open file handles
        return;
    }

    DWORD openMode;
    DWORD protection;
    DWORD mapMode;

    switch (mode) {
    case Mode::RDONLY:
        openMode = GENERIC_READ;
        protection = FILE_MAP_READ;
        mapMode = PAGE_READONLY;
        break;
    case Mode::RW:
        openMode = GENERIC_READ | GENERIC_WRITE;
        protection = FILE_MAP_READ | FILE_MAP_WRITE;
        mapMode = PAGE_READWRITE;
    }

    DWORD shared = FILE_SHARE_READ | FILE_SHARE_WRITE;
    filehandle = CreateFile(filename.c_str(),
                            openMode,
                            shared,
                            nullptr,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            nullptr);

    if (filehandle == INVALID_HANDLE_VALUE) {
        throw std::system_error(GetLastError(),
                                std::system_category(),
                                "cb::io::MemoryMappedFile(): CreateFile(" +
                                        filename + ") failed");
    }

    maphandle = CreateFileMapping(filehandle, nullptr, mapMode, 0, 0, nullptr);
    if (maphandle == INVALID_HANDLE_VALUE) {
        auto error = GetLastError();
        CloseHandle(filehandle);
        filehandle = INVALID_HANDLE_VALUE;
        throw std::system_error(
                error,
                std::system_category(),
                "cb::io::MemoryMappedFile(): CreateFileMapping() failed");
    }

    auto* root = MapViewOfFile(maphandle, protection, 0, 0, 0);
    if (root == nullptr) {
        auto error = GetLastError();
        CloseHandle(maphandle);
        maphandle = INVALID_HANDLE_VALUE;
        CloseHandle(filehandle);
        filehandle = INVALID_HANDLE_VALUE;

        throw std::system_error(
                error,
                std::system_category(),
                "cb::io::MemoryMappedFile(): MapViewOfFile() for file " +
                        filename + " failed");
    }

    mapping = {static_cast<char*>(root), size};
}

MemoryMappedFile::~MemoryMappedFile() {
    if (maphandle != INVALID_HANDLE_VALUE) {
        UnmapViewOfFile(static_cast<char*>(mapping.data()));
        CloseHandle(maphandle);
    }

    if (filehandle != INVALID_HANDLE_VALUE) {
        CloseHandle(filehandle);
    }
}

#else

MemoryMappedFile::MemoryMappedFile(const std::string& filename,
                                   const Mode& mode) {
    struct stat st;
    if (stat(filename.c_str(), &st) == -1) {
        throw std::system_error(errno,
                                std::system_category(),
                                "cb::io::MemoryMappedFile::open: stat(" +
                                        filename + ") failed");
    }
    auto size = size_t(st.st_size);

    if (size == 0) {
        // Empty file, no need to open file handles
        return;
    }

    int openMode;
    int protection = PROT_READ;
    bool validMode = false;

    switch (mode) {
    case Mode::RDONLY:
        openMode = O_RDONLY;
        validMode = true;
        break;
    case Mode::RW:
        openMode = O_RDWR;
        validMode = true;
        protection |= PROT_WRITE;
        break;
    }
    if (!validMode) {
        throw std::logic_error(
                "cb::io::MemoryMappedFile(): invalid value for mode:" +
                std::to_string(uint8_t(mode)));
    }

    if ((filehandle = ::open(filename.c_str(), openMode)) == -1) {
        throw std::system_error(
                errno,
                std::system_category(),
                "cb::io::MemoryMappedFile(): open(" + filename + ") failed");
    }

    auto* root = mmap(
            nullptr, size, protection, MAP_FILE | MAP_SHARED, filehandle, 0);
    if (root == MAP_FAILED) {
        auto error = errno;
        ::close(filehandle);
        filehandle = -1;
        throw std::system_error(error,
                                std::system_category(),
                                "cb::io::MemoryMappedFile(): mmap() of file " +
                                        filename + " failed");
    }

    mapping = {static_cast<char*>(root), size};
}

MemoryMappedFile::~MemoryMappedFile() {
    if (mapping.data() != nullptr) {
        munmap(static_cast<void*>(mapping.data()), mapping.size());
    }
    if (filehandle != -1) {
        ::close(filehandle);
    }
}

#endif

} // namespace io
} // namespace cb

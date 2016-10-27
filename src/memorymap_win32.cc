/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#include <platform/memorymap.h>
#include <platform/platform.h>
#include <sstream>
#include <stdexcept>
#include <system_error>

cb::MemoryMappedFile::MemoryMappedFile(const char* fname, const Mode& mode_)
    : filename(fname),
      filehandle(INVALID_HANDLE_VALUE),
      maphandle(INVALID_HANDLE_VALUE),
      root(NULL),
      size(0),
      mode(mode_) {
}

cb::MemoryMappedFile::~MemoryMappedFile() {
    close();
}

void cb::MemoryMappedFile::close(void) {
    /* file not mapped */
    if (root == NULL) {
        return;
    }
    std::stringstream ss;

    DWORD error = 0;
    if (!UnmapViewOfFile(root)) {
        error = GetLastError();
    }
    CloseHandle(maphandle);
    maphandle = INVALID_HANDLE_VALUE;
    CloseHandle(filehandle);
    filehandle = INVALID_HANDLE_VALUE;
    root = NULL;
    size = 0;

    if (error != 0) {
        throw std::system_error(
            error, std::system_category(),
            "cb::MemoryMappedFile::close: UnmapViewOfFile() failed");
    }
}

void cb::MemoryMappedFile::open(void) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &fad) ==
        0) {
        throw std::system_error(GetLastError(),
                                std::system_category(),
                                "cb::MemoryMappedFile::open: GetFileAttributesEx() failed");
    }
    LARGE_INTEGER sz;
    sz.HighPart = fad.nFileSizeHigh;
    sz.LowPart = fad.nFileSizeLow;
    size = (size_t)sz.QuadPart;

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
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (filehandle == INVALID_HANDLE_VALUE) {
        throw std::system_error(GetLastError(),
                                std::system_category(),
                                "cb::MemoryMappedFile::open: CreateFile() failed");
    }

    maphandle = CreateFileMapping(filehandle, nullptr, mapMode, 0, 0, nullptr);
    if (maphandle == INVALID_HANDLE_VALUE) {
        auto error = GetLastError();
        CloseHandle(filehandle);
        throw std::system_error(error, std::system_category(),
                                "cb::MemoryMappedFile::open: CreateFileMapping() failed");
    }

    root = MapViewOfFile(maphandle, protection, 0, 0, 0);
    if (root == NULL) {
        auto error = GetLastError();
        CloseHandle(maphandle);
        CloseHandle(filehandle);

        throw std::system_error(error, std::system_category(),
                                "cb::MemoryMappedFile::open: MapViewOfFile() failed");
    }
}

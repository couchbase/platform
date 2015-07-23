/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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
#include <platform/platform.h>

#include <sstream>
#include <platform/strerror.h>
#include "platform/memorymap.h"

Couchbase::MemoryMappedFile::MemoryMappedFile(const char *fname, bool share, bool rdonly)
        :
        filename(fname),
        filehandle(INVALID_HANDLE_VALUE),
        maphandle(INVALID_HANDLE_VALUE),
        root(NULL),
        size(0),
        sharedMapping(share),
        readonly(rdonly) {
}

Couchbase::MemoryMappedFile::~MemoryMappedFile() {
    close();
}

void Couchbase::MemoryMappedFile::close(void) {
    /* file not mapped */
    if (root == NULL) {
        return;
    }
    std::stringstream ss;

    if (!UnmapViewOfFile(root)) {
        ss << "UnmapViewOfFile() failed: " << cb_strerror();
    }
    CloseHandle(maphandle);
    maphandle = INVALID_HANDLE_VALUE;
    CloseHandle(filehandle);
    filehandle = INVALID_HANDLE_VALUE;
    root = NULL;
    size = 0;

    std::string str = ss.str();
    if (str.length() > 0) {
        throw str;
    }
}

void Couchbase::MemoryMappedFile::open(void) {
    if (sharedMapping && readonly) {
        throw std::string("Invalid mode: shared and readonly don't make sense");
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard,
            &fad) == 0) {
        std::stringstream ss;
        ss << "failed to determine file size: " << cb_strerror();
        throw ss.str();
    }
    LARGE_INTEGER sz;
    sz.HighPart = fad.nFileSizeHigh;
    sz.LowPart = fad.nFileSizeLow;
    size = (size_t) sz.QuadPart;

    DWORD mode;
    DWORD access;
    if (readonly) {
        mode = GENERIC_READ;
        access = FILE_MAP_READ;
    } else {
        mode = GENERIC_READ | GENERIC_WRITE;
        access = FILE_MAP_READ | FILE_MAP_WRITE;
    }

    DWORD shared = 0;
    if (sharedMapping) {
        shared = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

    filehandle = CreateFile(filename.c_str(), mode,
            shared, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);

    if (filehandle == INVALID_HANDLE_VALUE) {
        std::stringstream ss;
        ss << "failed to open file: " << cb_strerror();
        size = 0;
        throw ss.str();
    }

    maphandle = CreateFileMapping(filehandle, NULL,
            readonly ? PAGE_READONLY : PAGE_READWRITE,
            0, 0, NULL);
    if (maphandle == INVALID_HANDLE_VALUE) {
        std::stringstream ss;
        ss << "failed to create file mapping: " << cb_strerror();
        CloseHandle(filehandle);
        size = 0;
        throw ss.str();
    }

    root = MapViewOfFile(maphandle, access, 0, 0, 0);
    if (root == NULL) {
        std::stringstream ss;
        ss << "mapviewoffile failed: " << cb_strerror();
        CloseHandle(maphandle);
        CloseHandle(filehandle);
        size = 0;
        throw ss.str();
    }
}

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

#include <sys/mman.h>
#ifdef __sun
const int MAP_FILE = 0;
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include "platform/memorymap.h"

cb::MemoryMappedFile::MemoryMappedFile(const char* fname,
                                       bool share,
                                       bool rdonly)
    : filename(fname),
      filehandle(-1),
      root(NULL),
      size(0),
      sharedMapping(share),
      readonly(rdonly) {
    // Empty
}

cb::MemoryMappedFile::~MemoryMappedFile() {
    close();
}

void cb::MemoryMappedFile::close(void) {
    /* file not mapped anymore */
    if (root == NULL) {
        return;
    }
    int error = 0;

    if (munmap(root, size) != 0) {
        error = errno;
    }
    ::close(filehandle);
    filehandle = -1;
    root = NULL;
    size = 0;

    if (error != 0) {
        throw std::system_error(error, std::system_category(),
                                "cb::MemoryMappedFile::close: munmap() failed");
    }
}

void cb::MemoryMappedFile::open(void) {
    if (sharedMapping && readonly) {
        throw std::invalid_argument(
            "cb::MemoryMappedFile::open: Invalid mode: shared and readonly don't make sense");
    }

    struct stat st;
    if (stat(filename.c_str(), &st) == -1) {
        throw std::system_error(
            errno, std::system_category(),
            "cb::MemoryMappedFile::open: stat() failed");
    }
    size = st.st_size;

    int mapMode = MAP_FILE;
    int openMode;
    if (sharedMapping) {
        openMode = O_RDWR;
        mapMode |= MAP_SHARED;
    } else {
        openMode = O_RDONLY;
        mapMode |= MAP_PRIVATE;
    }

    int protection = PROT_READ;
    if (readonly) {
        openMode = O_RDONLY;
    } else {
        protection |= PROT_WRITE;
    }

    if ((filehandle = ::open(filename.c_str(), openMode)) == -1) {
        throw std::system_error(
            errno, std::system_category(), "cb::MemoryMappedFile::open: open() failed");
    }

    root = mmap(NULL, size, protection, mapMode, filehandle, 0);
    if (root == MAP_FAILED) {
        auto error = errno;
        ::close(filehandle);
        filehandle = -1;
        root = NULL;
        size = 0;
        throw std::system_error(
            error, std::system_category(), "cb::MemoryMappedFile::open: mmap() failed");
    }
}

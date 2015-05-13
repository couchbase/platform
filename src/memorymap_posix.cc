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

#include <sys/mman.h>
#ifdef __sun
const int MAP_FILE = 0;
#endif

#include <sstream>
#include <cerrno>
#include <cstring>
#include "platform/memorymap.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

Couchbase::MemoryMappedFile::MemoryMappedFile(const char *fname, bool share, bool rdonly) :
        filename(fname),
        filehandle(-1),
        root(NULL),
        size(0),
        sharedMapping(share),
        readonly(rdonly) {
    // Empty
}

Couchbase::MemoryMappedFile::~MemoryMappedFile() {
    close();
}

void Couchbase::MemoryMappedFile::close(void) {
    /* file not mapped anymore */
    if (root == NULL) {
        return;
    }
    std::stringstream ss;

    if (munmap(root, size) != 0) {
        ss << "munmap failed: " << strerror(errno);
    }
    ::close(filehandle);
    filehandle = -1;
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

    struct stat st;
    if (stat(filename.c_str(), &st) == -1) {
        std::stringstream ss;
        ss << "stat(" << filename << ") failed: " << strerror(errno);
        throw ss.str();
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
        std::stringstream ss;
        ss << "Failed to open file: " << filename << " (" << strerror(errno) << ")";
        throw ss.str();
    }

    root = mmap(NULL, size, protection, mapMode, filehandle, 0);
    if (root == MAP_FAILED) {
        std::stringstream ss;
        ss << "mmap failed: " << strerror(errno);
        ::close(filehandle);
        filehandle = -1;
        root = NULL;
        size = 0;
        throw ss.str();
    }
}

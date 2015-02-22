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

#pragma once

#include <platform/visibility.h>

#include <inttypes.h>
#include <stddef.h>
#include <cstdio>
#include <string>

namespace Couchbase {
    class PLATFORM_PUBLIC_API MemoryMappedFile {
    public:
        ~MemoryMappedFile();

        MemoryMappedFile(const char *fname, bool share, bool rdonly);

        /**
        * Open the mapping. Throws an std::string with a reason why
        * in case of a failure.
        */
        void open(void);

        /**
        * Close the file mapping.. This invalidates the root pointer
        * and the mapping should NOT be used after it is closed
        * (it'll most likely cause a crash)
        */
        void close(void);

        /**
        * Get the address for the beginning of the pointer.
        */
        void *getRoot(void) const {
            if (root == NULL) {
                throw std::string("Internal error, open() not called");
            }
            return root;
        }

        /**
        * Get the size of the mapped segment
        */
        size_t getSize(void) const {
            if (root == NULL) {
                throw std::string("Internal error, open() not called");
            }
            return size;
        }

    private:
        MemoryMappedFile(MemoryMappedFile &) = delete;

        std::string filename;
#ifdef WIN32
        HANDLE filehandle;
        HANDLE maphandle;
#else
        int filehandle;
#endif
        void *root;
        size_t size;
        bool sharedMapping;
        bool readonly;
    };
}

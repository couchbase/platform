/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

#include <platform/platform.h>
#include <platform/sized_buffer.h>

#include <cinttypes>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

namespace cb {
namespace io {

/**
 * A class that implements memory mapping of a file. It allows for only
 * two different modes: Just read and read + write.
 *
 * All mappings are shared to other processes
 */
class PLATFORM_PUBLIC_API MemoryMappedFile {
public:
    enum class Mode : uint8_t {
        /// Open the map with only read access
        RDONLY,
        /// Open the map with read and write access
        RW
    };
    ~MemoryMappedFile();
    MemoryMappedFile(const std::string& fname, const Mode& mode_);

    MemoryMappedFile() = delete;
    MemoryMappedFile(MemoryMappedFile&) = delete;
    MemoryMappedFile(MemoryMappedFile&&) = delete;

    cb::char_buffer content() {
        return mapping;
    }

    cb::const_char_buffer content() const {
        return {mapping.data(), mapping.size()};
    }

protected:
#ifdef WIN32
    HANDLE filehandle = INVALID_HANDLE_VALUE;
    HANDLE maphandle = INVALID_HANDLE_VALUE;
#else
    int filehandle = -1;
#endif
    cb::char_buffer mapping = {};
};

} // namespace io

/**
 * For source compatibility we'll keep this class around while we move over
 * to the RIAA version
 */
class PLATFORM_PUBLIC_API MemoryMappedFile {
public:
    enum class Mode : uint8_t {
    /// Open the map with only read access
    RDONLY,
    /// Open the map with read and write access
    RW
    };
    MemoryMappedFile() = delete;
    MemoryMappedFile(MemoryMappedFile&) = delete;
    MemoryMappedFile(const char* fname, const Mode& mode_)
        : filename(fname), mode(mode_) {
    }

    /**
     * Open the mapping.
     * @throws std::exception (or one of the subclasses) if anything
     *                        goes wrong
     */
    void open() {
        switch (mode) {
        case Mode::RDONLY:
            mapping = std::make_unique<cb::io::MemoryMappedFile>(
                    filename, cb::io::MemoryMappedFile::Mode::RDONLY);
            break;
        case Mode::RW:
            mapping = std::make_unique<cb::io::MemoryMappedFile>(
                    filename, cb::io::MemoryMappedFile::Mode::RW);
            break;
        }
    }

    /**
     * Close the file mapping.. This invalidates the root pointer
     * and the mapping should NOT be used after it is closed
     * (it'll most likely cause a crash)
     *
     * @throws std::exception (or one of the subclasses) if anything
     *                        goes wrong
     */
    void close() {
        mapping.reset();
    }

    /**
     * Get the address for the beginning of the pointer.
     *
     */
    void* getRoot() const {
        if (!mapping) {
            throw std::logic_error(
                "cb::MemoryMappedFile::getRoot(): open() not called");
        }

        return static_cast<void*>(mapping->content().data());
    }

    /**
     * Get the size of the mapped segment
     */
    size_t getSize() const {
        if (!mapping) {
            throw std::logic_error(
                "cb::MemoryMappedFile::getSize(): open() not called");
        }
        return mapping->content().size();
    }

protected:
    std::string filename;
    const Mode mode;
    std::unique_ptr<cb::io::MemoryMappedFile> mapping;
};
}

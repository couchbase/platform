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
#include <platform/cbassert.h>
#include <platform/memorymap.h>
#include <platform/random.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include "config.h"

std::string filename;

static std::vector<uint8_t> readFile() {
    std::vector<uint8_t> ret;
    FILE* fp = fopen(filename.c_str(), "rb");
    cb_assert(fp != nullptr);
    cb_assert(fseek(fp, 0, SEEK_END) == 0);
    ret.resize(ftell(fp));
    cb_assert(fseek(fp, 0, SEEK_SET) == 0);
    cb_assert(fread(ret.data(), 1, ret.size(), fp) == ret.size());
    fclose(fp);

    return ret;
}

using MemoryMappedFile = cb::io::MemoryMappedFile;
using Mode = cb::io::MemoryMappedFile::Mode;

static void testReadonlyMapping() {
    std::vector<uint8_t> before = readFile();
    MemoryMappedFile mymap(filename, Mode::RDONLY);
    auto content = mymap.content();
    cb_assert(memcmp(before.data(), content.data(), content.size()) == 0);
}

static void testSharedMapping() {
    std::vector<uint8_t> before = readFile();
    MemoryMappedFile mymap(filename.c_str(), Mode::RW);
    auto content = mymap.content();
    auto* block = new uint8_t[content.size()];
    memset(block, 0, content.size());
    std::fill(content.begin(), content.end(), 0);
    cb_assert(memcmp(block, content.data(), content.size()) == 0);
    delete[] block;
    std::vector<uint8_t> after = readFile();
    cb_assert(before.size() == after.size());
    cb_assert(memcmp(before.data(), after.data(), before.size()) != 0);
}

static void createFile() {
    std::vector<uint8_t> buffer;
    buffer.resize(16 * 1024);
    Couchbase::RandomGenerator generator(false);
    generator.getBytes(buffer.data(), buffer.size());

    std::stringstream fnm;
    fnm << "memorymap-" << cb_getpid() << ".txt";
    filename = fnm.str();

    FILE* fp = fopen(fnm.str().c_str(), "w");
    cb_assert(fp != nullptr);
    cb_assert(fwrite(buffer.data(), 1, buffer.size(), fp) == buffer.size());
    fclose(fp);
}

int main() {
    createFile();
    testReadonlyMapping();
    testSharedMapping();
    remove(filename.c_str());
    exit(EXIT_SUCCESS);
}
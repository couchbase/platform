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

#include <stdlib.h>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstddef>
#include <platform/random.h>
#include <platform/memorymap.h>
#include <platform/cbassert.h>

using namespace Couchbase;
std::string filename;

static std::vector<uint8_t> readFile(void) {
    std::vector<uint8_t> ret;
    FILE *fp = fopen(filename.c_str(), "rb");
    cb_assert(fp != NULL);
    cb_assert(fseek(fp, 0, SEEK_END) == 0);
    ret.resize(ftell(fp));
    cb_assert(fseek(fp, 0, SEEK_SET) == 0);
    cb_assert(fread(ret.data(), 1, ret.size(), fp) == ret.size());
    fclose(fp);

    return ret;
}

static void testInvalidMapOptions(void) {
    MemoryMappedFile mymap(filename.c_str(), true, true);
    try {
        mymap.open();
        std::cerr << "ERROR: readonly mapping with sharing doesn't make sense " << std::endl;
        exit(EXIT_FAILURE);
    } catch (std::string err) {
    }
}

static void testReadonlyMapping(void) {
    std::vector<uint8_t> before = readFile();
    MemoryMappedFile mymap(filename.c_str(), false, true);
    try {
        mymap.open();
    } catch (std::string err) {
        std::cerr << "ERROR: " << err << std::endl;
        exit(EXIT_FAILURE);
    }
    cb_assert(memcmp(before.data(), mymap.getRoot(), mymap.getSize()) == 0);
}

static void testPrivateMapping(void) {
    std::vector<uint8_t> before = readFile();
    MemoryMappedFile mymap(filename.c_str(), false, false);
    try {
        mymap.open();
    } catch (std::string err) {
        std::cerr << "ERROR: " << err << std::endl;
        exit(EXIT_FAILURE);
    }
    uint8_t *block = new uint8_t[mymap.getSize()];
    memset(block, 0, mymap.getSize());
    memset(mymap.getRoot(), 0, mymap.getSize());
    cb_assert(memcmp(block, mymap.getRoot(), mymap.getSize()) == 0);
    delete[]block;
    std::vector<uint8_t> after = readFile();
    cb_assert(before.size() == after.size());
    cb_assert(memcmp(before.data(), after.data(), before.size()) == 0);
}

static void testSharedMapping(void) {
    std::vector<uint8_t> before = readFile();
    MemoryMappedFile mymap(filename.c_str(), true, false);
    try {
        mymap.open();
    } catch (std::string err) {
        std::cerr << "ERROR: " << err << std::endl;
        exit(EXIT_FAILURE);
    }
    uint8_t *block = new uint8_t[mymap.getSize()];
    memset(block, 0, mymap.getSize());
    memset(mymap.getRoot(), 0, mymap.getSize());
    cb_assert(memcmp(block, mymap.getRoot(), mymap.getSize()) == 0);
    delete[]block;
    std::vector<uint8_t> after = readFile();
    cb_assert(before.size() == after.size());
    cb_assert(memcmp(before.data(), after.data(), before.size()) != 0);
}

static void createFile(void) {
    std::vector<uint8_t> buffer;
    buffer.resize(16 * 1024);
    RandomGenerator generator(false);
    generator.getBytes(buffer.data(), buffer.size());

    std::stringstream fnm;
    fnm << "memorymap-" << getpid() << ".txt";
    filename = fnm.str();

    FILE *fp = fopen(fnm.str().c_str(), "w");
    cb_assert(fp != NULL);
    cb_assert(fwrite(buffer.data(), 1, buffer.size(), fp) == buffer.size());
    fclose(fp);
}

int main(void) {
    createFile();
    testInvalidMapOptions();
    testReadonlyMapping();
    testPrivateMapping();
    testSharedMapping();
    remove(filename.c_str());
    exit(EXIT_SUCCESS);
}
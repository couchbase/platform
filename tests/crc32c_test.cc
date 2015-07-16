/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

//
// Test CRC32C using the IETF CRC32C examples
//  - https://tools.ietf.org/html/rfc3720#appendix-B.4
//

#ifndef CRC32C_UNIT_TEST
#include "platform/crc32c.h"
#endif

#include <assert.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <vector>

typedef uint32_t (*test_function)(uint8_t* buf, size_t len);

// The test vector functions intialise the buffer and return the expected CRC32C
uint32_t zero_32(uint8_t* buf, size_t len) {
    assert(len == 32);
    std::memset(buf, 0, 32);
    return 0x8a9136aa;
}

uint32_t ones_32(uint8_t* buf, size_t len) {
    assert(len == 32);
    std::memset(buf, 0xff, 32);
    return 0x62a8ab43;
}

uint32_t incrementing_32(uint8_t* buf, size_t len) {
    assert(len == 32);
    for (size_t ii = 0; ii < len; ii++) {
        buf[ii] = static_cast<uint8_t>(ii);
    }
    return 0x46dd794e;
}

uint32_t decrementing_32(uint8_t* buf, size_t len) {
    assert(len == 32);
    uint8_t val = static_cast<uint8_t>(len-1);
    for (size_t ii = 0; ii < len; ii++) {
        buf[ii] = val--;
    }
    return 0x113fdb5c;
}

uint32_t iscsi_read(uint8_t* buf, size_t len) {
    assert(len == 48);
    uint8_t iscsi_read_data[48] =
       {0x01,0xc0,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x14,0x00,0x00,0x00,
        0x00,0x00,0x04,0x00,
        0x00,0x00,0x00,0x14,
        0x00,0x00,0x00,0x18,
        0x28,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x02,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00};

    std::memcpy(buf, iscsi_read_data, 48);
    return 0xd9963a56;
}

//
// Not a test from ietf, but a long data test
// as the crc32c functions maybe optimised for >8192
// 1Mb buffer
//
uint32_t long_data(uint8_t* buf, size_t len) {
    assert(len == 1024*1024);
    std::memset(buf, 0x7a, len);
    buf += 1024;
    len -= 1024;
    while(len > 0) {
       iscsi_read(buf, 48);
       buf+=48;
       len-=48;
    }
    return 0x93a9ae7a;
}

uint32_t short_data(uint8_t* buf, size_t len) {
    assert(len == 8000);
    std::memset(buf, 0x7a, len);
    buf += 1024;
    len -= 1024;
    while(len > 48) {
       iscsi_read(buf, 48);
       buf+=48;
       len-=48;
    }
    return 0x9966c079;
}

bool run_test(const uint8_t* buffer, int len, uint32_t expected, std::string name) {
    uint32_t actual = 0;

#ifdef CRC32C_UNIT_TEST
    // extern directly to the hw/sw versions
    extern uint32_t crc32c_sw(const uint8_t* buf, size_t len, uint32_t crc_in);
    extern uint32_t crc32c_hw(const uint8_t* buf, size_t len, uint32_t crc_in);
    extern uint32_t crc32c_hw_1way(const uint8_t* buf, size_t len, uint32_t crc_in);
    // in the unit test version, we're bypassing the DLL exposed interface
    // and running hard/software function together for full validation.
    actual = crc32c_hw_1way(buffer, len, 0) & crc32c_sw(buffer, len, 0) & crc32c_hw(buffer, len, 0);
#else
    actual = crc32c(buffer, len, 0);
#endif

    if (expected != actual) {
        std::cerr << "Test " << name << ": failed. Expected crc "
            << std::hex << expected << " != actual crc "
            << actual << std::dec << std::endl;
    }
    return expected == actual;
}

bool run_test_function(uint8_t* buffer, int len, test_function test, std::string name) {
    uint32_t expected = test(buffer, len);
    return run_test(buffer, len, expected, name);
}


int main() {
    uint8_t* buffer = new uint8_t[33];
    bool pass = true;
    pass &= run_test_function(buffer, 32, zero_32, "Zero 32");
    pass &= run_test_function(buffer+1, 32, zero_32, "Zero 32 - unaligned");
    pass &= run_test_function(buffer, 32, ones_32, "Ones 32");
    pass &= run_test_function(buffer+1, 32, ones_32, "Ones 32 - unaligned");
    pass &= run_test_function(buffer, 32, incrementing_32, "Incr 32");
    pass &= run_test_function(buffer+1, 32, incrementing_32, "Incr 32 - unaligned");
    pass &= run_test_function(buffer+1, 32, decrementing_32, "Decr 32 - unaligned");
    pass &= run_test_function(buffer, 32, decrementing_32, "Decr 32");
    delete [] buffer;
    buffer = new uint8_t[49];
    pass &= run_test_function(buffer, 48, iscsi_read, "ISCSI read");
    pass &= run_test_function(buffer+1, 48, iscsi_read, "ISCSI read - unaligned");
    delete [] buffer;
    buffer = new uint8_t[(1024*1024)+1];
    pass &= run_test_function(buffer, 1024*1024, long_data, "long data");
    pass &= run_test_function(buffer+1, 1024*1024, long_data, "long data - unaligned");

    // we have optimisation at >3x8192 boundary
    pass &= run_test(buffer, (3*8192) + 65, 0x80536521, "1 long block + 65");
    pass &= run_test(buffer+1, (3*8192) + 65, 0xa3a771a, "1 long block + 65 - unaligned");
    pass &= run_test(buffer, (6*8192) + 65, 0x1bda16e9, "2x long block + 65");
    pass &= run_test(buffer+1, (6*8192) + 65, 0x9a57a5e2, "2x long block + 65 - unaligned");
    pass &= run_test(buffer, (3*8192) + (3*256) + 65, 0xe7b2487a,
                     "1 long + 1 short + 65");
    pass &= run_test(buffer+1, (3*8192) + (3*256) + 65, 0x64ad9ac7,
                     "1 long + 1 short + 65 - unaligned");

    pass &= run_test_function(buffer, 8000, short_data, "short data");
    pass &= run_test_function(buffer+1, 8000, short_data, "short data - unaligned");

    // we have optimisation at >3x256 boundary
    pass &= run_test(buffer, (256*3) + 65, 0x850d4115, "1x short block + 65");
    pass &= run_test(buffer+3, (256*3) + 65, 0x850d4115, "1x short block + 65 - unaligned");
    pass &= run_test(buffer, (256*6) + 65, 0x92819a69, "2x short block + 65");
    pass &= run_test(buffer+3, (256*6) + 65, 0x3ab67f68, "2x short block + 65 - unaligned");

    // Test sizes 0 to 8 bytes.
    // These are the precomputed results (checked against two different crc32c implementations)
    // Input data is the decrementing_32 buffer.
    std::vector<uint32_t> results = {
        0, 0, 0x1c30a81a, 0xee5b2b19,
        0x95fbf4e6, 0x95099f65, 0x380f6ceb, 0x5bc2d50f,
        0x75e157a3, 0x85e2c1f4, 0x6ac49800, 0x1db0a6ad,
        0x29ab789e, 0x27ef0621, 0x0468a7ba, 0x1a0b2e2a,
        0x1109fea7, 0xddac5d1d
    };
    // repopulate the data buffer
    pass &= run_test_function(buffer, 32, decrementing_32, "Decr 32");
    for (uint32_t ii = 0, res = 0; ii <= sizeof(uint64_t); res += 2, ii++) {
        std::string size = std::to_string(ii);
        pass &= run_test(buffer, ii, results[res], size + " bytes");
        pass &= run_test(buffer+1, ii, results[res+1], size + " bytes - unaligned");
    }


    delete [] buffer;
    return pass ? 0 : 1;
}

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
// Benchmark the crc32c functions
// We extern the symbols directly so software and the tuned/untuned
// hardware versions can be checked.
//

#include <stdint.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <random>
#include <string>
#include <iomanip>
#include <sstream>

#include "platform/platform.h"

// extern directly to the hw/sw versions
extern uint32_t crc32c_sw(const uint8_t* buf, size_t len, uint32_t crc_in);
extern uint32_t crc32c_hw(const uint8_t* buf, size_t len, uint32_t crc_in);
extern uint32_t crc32c_hw_1way(const uint8_t* buf, size_t len, uint32_t crc_in);

typedef uint32_t (*crc32c_function)(const uint8_t* buf, size_t len, uint32_t crc_in);

static std::vector<std::string> column_heads(0);

void crc_results_banner() {
    column_heads.push_back("Data size (bytes) ");
    column_heads.push_back("SW ns      ");
    column_heads.push_back("SW GiB/s   ");
    column_heads.push_back("HW ns      ");
    column_heads.push_back("HW GiB/s   ");
    column_heads.push_back("HW vs SW ");
    column_heads.push_back("HW opt ns  ");
    column_heads.push_back("HW opt GiB/s ");
    column_heads.push_back("HW vs HW opt ");
    for (auto str : column_heads) {
        std::cout << str << ": ";
    }
    std::cout << std::endl;
}

std::string gib_per_sec(size_t test_size, hrtime_t t) {
    uint64_t one_sec = 1000000000;
    uint64_t how_many_per_sec = one_sec / t;
    double bytes_per_sec = test_size * how_many_per_sec;
    double gib_per_sec = bytes_per_sec/(1024.0*1024.0*1024.0);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3) << gib_per_sec;
    return ss.str();
}

void crc_results(size_t test_size,
                 std::vector<hrtime_t> &timings_sw,
                 std::vector<hrtime_t> &timings_hw,
                 std::vector<hrtime_t> &timings_hw_opt) {
    hrtime_t avg_sw = 0, avg_hw = 0, avg_hw_opt = 0;
    for(auto duration : timings_sw) {
        avg_sw += duration;
    }
    for(auto duration : timings_hw) {
        avg_hw += duration;
    }
    for(auto duration : timings_hw_opt) {
        avg_hw_opt += duration;
    }
    avg_sw = avg_sw / timings_sw.size();
    avg_hw = avg_hw / timings_hw.size();
    avg_hw_opt = avg_hw_opt / timings_hw.size();

    double sw_hw = static_cast<double>(avg_sw) / static_cast<double>(avg_hw);
    double hw_hw_opt = static_cast<double>(avg_hw) / static_cast<double>(avg_hw_opt);

    std::vector<std::string> rows(0);
    rows.push_back(std::to_string(test_size));
    rows.push_back(std::to_string(avg_sw));
    rows.push_back(gib_per_sec(test_size, avg_sw));
    rows.push_back(std::to_string(avg_hw));
    rows.push_back(gib_per_sec(test_size, avg_hw));
    rows.push_back(std::to_string(sw_hw));
    rows.push_back(std::to_string(avg_hw_opt));
    rows.push_back(gib_per_sec(test_size, avg_hw_opt));
    rows.push_back(std::to_string(hw_hw_opt));

    for (size_t ii = 0; ii < column_heads.size(); ii++) {
        std::string spacer(column_heads[ii].length() - rows[ii].length(), ' ');
        std::cout << rows[ii] << spacer << ": ";
    }
    std::cout << std::endl;
}

void crc_bench_core(const uint8_t* buffer,
                    size_t len,
                    int iterations,
                    crc32c_function crc32c_fn,
                    std::vector<hrtime_t> &timings) {
    for (int i = 0; i < iterations; i++) {
        const hrtime_t start = gethrtime();
        crc32c_fn(buffer, len, 0);
        const hrtime_t end = gethrtime();
        timings.push_back(end - start);
    }
}

void crc_bench(size_t len,
               int iterations,
               int unalignment) {
    uint8_t* data = new uint8_t[len+unalignment];
    std::mt19937 twister(static_cast<int>(len));
    std::uniform_int_distribution<> dis(0, 0xff);
    for (size_t data_index = 0; data_index < len; data_index++) {
        uint8_t data_value = static_cast<uint8_t>(dis(twister));
        data[data_index] = data_value;
    }
    std::vector<hrtime_t> timings_sw, timings_hw, timings_hw_opt;
    crc_bench_core(data+unalignment, len, iterations, crc32c_sw, timings_sw);
    crc_bench_core(data+unalignment, len, iterations, crc32c_hw_1way, timings_hw);
    crc_bench_core(data+unalignment, len, iterations, crc32c_hw, timings_hw_opt);
    delete [] data;

    crc_results(len, timings_sw, timings_hw, timings_hw_opt);

}

int main() {
    crc_results_banner();

    // test up to 8Mb
    std::cout << "Power of 2 lengths." << std::endl;
    for(size_t size = 32; size <= 8*(1024*1024); size = size * 4) {
        crc_bench(size, 1000, 0);
    }
    std::cout << std::endl;

    // Test some non power of 2 input sizes
    std::cout << "Non-power of 2 lengths." << std::endl;
    for(size_t size = 33; size <= 8*(1024*1024); size = size * 4) {
        crc_bench(size, 1000, 0);
    }
    std::cout << std::endl;

    // Test some inputs that are odd lengths and unaligned pointers
    std::cout << "Unaligned buffer of odd lengths" << std::endl;
    for(size_t size = 33; size <= 8*(1024*1024); size = size * 4) {
        crc_bench(size % 2 == 0? size+1:size, 1000, 1);
    }
    return 0;
}
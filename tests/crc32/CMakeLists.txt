cmake_minimum_required(VERSION 3.15)

# Tests all available implementations (software, and hardware where
# CB_CRC32_HW_SUPPORTED is defined by platform/crc32c.h)
cb_add_test_executable(platform-crc32c-test crc32c_test.cc)
target_link_libraries(platform-crc32c-test PRIVATE
                      platform
                      GTest::gtest
                      GTest::gtest_main)
platform_enable_pch(platform-crc32c-test)
add_test(platform-crc32c-test platform-crc32c-test)

cb_add_test_executable(platform-crc32c-bench crc32c_bench.cc)
target_link_libraries(platform-crc32c-bench PRIVATE platform)
platform_enable_pch(platform-crc32c-bench)

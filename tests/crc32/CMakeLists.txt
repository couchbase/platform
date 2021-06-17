cmake_minimum_required(VERSION 3.15)

cb_add_test_executable(platform-crc32c-test crc32c_test.cc)
target_link_libraries(platform-crc32c-test PRIVATE platform)
platform_enable_pch(platform-crc32c-test)

set(crc32_hw_archs "AMD64;x86_64")
if(CMAKE_SYSTEM_PROCESSOR IN_LIST crc32_hw_archs)
    cb_add_test_executable(platform-crc32c-sw_hw-test crc32c_test.cc)
    target_compile_definitions(platform-crc32c-sw_hw-test PRIVATE CRC32C_UNIT_TEST)
    target_link_libraries(platform-crc32c-sw_hw-test PRIVATE platform)
    platform_enable_pch(platform-crc32c-sw_hw-test)

    add_test(platform-crc32c-sw_hw-test platform-crc32c-sw_hw-test)
endif()

cb_add_test_executable(platform-crc32c-bench crc32c_bench.cc)
target_link_libraries(platform-crc32c-bench PRIVATE platform)
platform_enable_pch(platform-crc32c-bench)

add_test(platform-crc32c-test platform-crc32c-test)

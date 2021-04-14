add_executable(platform-crc32c-test crc32c_test.cc)
target_link_libraries(platform-crc32c-test platform)

add_executable(platform-crc32c-sw_hw-test crc32c_test.cc)
target_link_libraries(platform-crc32c-sw_hw-test platform)

add_executable(platform-crc32c-bench crc32c_bench.cc)
target_link_libraries(platform-crc32c-bench platform)

add_test(platform-crc32c-test platform-crc32c-test)
add_test(platform-crc32c-sw_hw-test platform-crc32c-sw_hw-test)

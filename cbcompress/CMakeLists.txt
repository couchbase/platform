add_library(cbcompress STATIC
            ${Platform_SOURCE_DIR}/include/platform/compress.h
            ${Platform_SOURCE_DIR}/include/platform/compression/allocator.h
            ${Platform_SOURCE_DIR}/include/platform/compression/buffer.h
            compress.cc)
set_target_properties(cbcompress PROPERTIES POSITION_INDEPENDENT_CODE true)
target_include_directories(cbcompress SYSTEM PRIVATE ${SNAPPY_INCLUDE_DIR})
target_include_directories(cbcompress INTERFACE ${Platform_SOURCE_DIR}/include)
platform_enable_pch(cbcompress)
target_link_libraries(cbcompress
        PUBLIC Folly::headers fmt::fmt
        PRIVATE platform ${SNAPPY_LIBRARIES} ZLIB::ZLIB)

cb_add_test_executable(platform-compression-test
               ${Platform_SOURCE_DIR}/include/platform/compress.h
               compression_test.cc)
target_link_libraries(platform-compression-test
                      PRIVATE
                      cbcompress
                      GTest::gtest
                      GTest::gtest_main)
platform_enable_pch(platform-compression-test)
add_test(platform-compression-test platform-compression-test)

cb_add_test_executable(platform-compression-bench compress_bench.cc)
target_include_directories(platform-compression-bench
                           SYSTEM PRIVATE
                           ${SNAPPY_INCLUDE_DIR})
target_link_libraries(platform-compression-bench
                      PRIVATE
                      ${SNAPPY_LIBRARIES}
                      benchmark::benchmark
                      GTest::gtest)

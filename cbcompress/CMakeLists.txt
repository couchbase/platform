add_library(cbcompress STATIC
            ${Platform_SOURCE_DIR}/include/platform/compress.h
            ${Platform_SOURCE_DIR}/include/platform/compression/allocator.h
            ${Platform_SOURCE_DIR}/include/platform/compression/buffer.h
            compress.cc)
set_target_properties(cbcompress PROPERTIES POSITION_INDEPENDENT_CODE true)
target_include_directories(cbcompress
                           PRIVATE
                           ${SNAPPY_INCLUDE_DIR})

platform_enable_pch(cbcompress)

target_link_libraries(cbcompress
                      PRIVATE
                      gsl::gsl-lite
                      platform_cbassert
                      platform_headers
                      ${SNAPPY_LIBRARIES}
                      )

cb_add_test_executable(platform-compression-test
               ${Platform_SOURCE_DIR}/include/platform/compress.h
               compression_test.cc)

target_link_libraries(platform-compression-test
                      PRIVATE
                      cbcompress
                      GTest::gtest
                      GTest::gtest_main
                      platform)
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
platform_enable_pch(platform-compression-bench)

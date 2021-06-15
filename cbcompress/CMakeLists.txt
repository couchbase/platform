# LZ4 tags deprecated methods with a macro. Current version of LZ4
# (v.1.8.0) gives an error under Clang when -std=c++14 is specified -
# see https://github.com/lz4/lz4/pull/393. Disable the macro to avoid
# this.
if (LZ4_FOUND)
  set_property(DIRECTORY APPEND PROPERTY
    COMPILE_DEFINITIONS "LZ4_DISABLE_DEPRECATE_WARNINGS")
endif()

add_library(cbcompress STATIC
            ${Platform_SOURCE_DIR}/include/platform/compress.h
            ${Platform_SOURCE_DIR}/include/platform/compression/allocator.h
            ${Platform_SOURCE_DIR}/include/platform/compression/buffer.h
            compress.cc)
set_target_properties(cbcompress PROPERTIES POSITION_INDEPENDENT_CODE true)
target_compile_definitions(cbcompress
                           PRIVATE
                           $<$<BOOL:${LZ4_FOUND}>:CB_LZ4_SUPPORT>)

target_include_directories(cbcompress
                           PRIVATE
                           ${SNAPPY_INCLUDE_DIR}
                           $<$<BOOL:${LZ4_FOUND}>:${LZ4_INCLUDE_DIR}>)

platform_enable_pch(cbcompress)

target_link_libraries(cbcompress
                      gsl_lite
                      platform_cbassert
                      platform_headers
                      ${SNAPPY_LIBRARIES}
                      $<$<BOOL:${LZ4_FOUND}>:${LZ4_LIBRARIES}>)

if (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
    #
    # Unit tests
    #
    cb_add_test_executable(platform-compression-test
                   ${Platform_SOURCE_DIR}/include/platform/compress.h
                   compression_test.cc)
    target_compile_definitions(platform-compression-test
                               PRIVATE
                               $<$<BOOL:${LZ4_FOUND}>:CB_LZ4_SUPPORT>)

    target_link_libraries(platform-compression-test
                          cbcompress
                          gtest
                          gtest_main
                          platform)
    platform_enable_pch(platform-compression-test)
    add_test(platform-compression-test platform-compression-test)

    #
    # Benchmark
    #
    cb_add_test_executable(platform-compression-bench compress_bench.cc)
    target_compile_definitions(platform-compression-bench
                               PRIVATE
                               $<$<BOOL:${LZ4_FOUND}>:CB_LZ4_SUPPORT>)
    target_include_directories(platform-compression-bench
                               SYSTEM PRIVATE
                               ${SNAPPY_INCLUDE_DIR}
                               $<$<BOOL:${LZ4_FOUND}>:${LZ4_INCLUDE_DIR}>
                               ${benchmark_SOURCE_DIR}/include)
    target_link_libraries(platform-compression-bench
                          ${SNAPPY_LIBRARIES}
                          $<$<BOOL:${LZ4_FOUND}>:${LZ4_LIBRARIES}>
                          benchmark
                          gtest)
    platform_enable_pch(platform-compression-bench)
endif (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)

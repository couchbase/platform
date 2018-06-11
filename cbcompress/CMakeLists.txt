# LZ4 tags deprecated methods with a macro. Current version of LZ4
# (v.1.8.0) gives an error under Clang when -std=c++14 is specified -
# see https://github.com/lz4/lz4/pull/393. Disable the macro to avoid
# this.
if (LZ4_FOUND)
  set_property(DIRECTORY APPEND PROPERTY
    COMPILE_DEFINITIONS "LZ4_DISABLE_DEPRECATE_WARNINGS")
endif()

add_library(cbcompress SHARED
            ${Platform_SOURCE_DIR}/include/platform/compress.h
            ${Platform_SOURCE_DIR}/include/platform/compression/allocator.h
            ${Platform_SOURCE_DIR}/include/platform/compression/buffer.h
            compress.cc)
target_compile_definitions(cbcompress
                           PRIVATE
                           $<$<BOOL:${LZ4_FOUND}>:CB_LZ4_SUPPORT>)

target_include_directories(cbcompress
                           PRIVATE
                           ${SNAPPY_INCLUDE_DIR}
                           $<$<BOOL:${LZ4_FOUND}>:${LZ4_INCLUDE_DIR}>)

target_link_libraries(cbcompress gsl_lite platform
                      ${SNAPPY_LIBRARIES}
                      $<$<BOOL:${LZ4_FOUND}>:${LZ4_LIBRARIES}>)

generate_export_header(cbcompress
                       EXPORT_MACRO_NAME CBCOMPRESS_PUBLIC_API
                       EXPORT_FILE_NAME ${Platform_BINARY_DIR}/include/platform/compress-visibility.h)

install(TARGETS cbcompress
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

if (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
    #
    # Unit tests
    #
    add_executable(platform-compression-test
                   ${Platform_SOURCE_DIR}/include/platform/compress.h
                   compression_test.cc)
    target_compile_definitions(platform-compression-test
                               PRIVATE
                               $<$<BOOL:${LZ4_FOUND}>:CB_LZ4_SUPPORT>)

    target_link_libraries(platform-compression-test cbcompress gtest gtest_main)
    add_test(platform-compression-test platform-compression-test)

    #
    # Benchmark
    #
    add_executable(platform-compression-bench compress_bench.cc)
    target_compile_definitions(platform-compression-bench
                               PRIVATE
                               $<$<BOOL:${LZ4_FOUND}>:CB_LZ4_SUPPORT>)
    target_include_directories(platform-compression-bench
                               PRIVATE
                               ${SNAPPY_INCLUDE_DIR}
                               $<$<BOOL:${LZ4_FOUND}>:${LZ4_INCLUDE_DIR}>)
    target_link_libraries(platform-compression-bench
                          ${SNAPPY_LIBRARIES}
                          $<$<BOOL:${LZ4_FOUND}>:${LZ4_LIBRARIES}>
                          benchmark
                          gtest)
endif (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)

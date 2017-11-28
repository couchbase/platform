include_directories(AFTER ${SNAPPY_INCLUDE_DIR})

list(APPEND cbcompress_libraries platform ${SNAPPY_LIBRARIES})

if (LZ4_FOUND)
    add_definitions(-DCB_LZ4_SUPPORT)
    include_directories(AFTER ${LZ4_INCLUDE_DIR})
    list(APPEND cbcompress_libraries ${LZ4_LIBRARIES})
endif (LZ4_FOUND)

add_library(cbcompress SHARED
            ${Platform_SOURCE_DIR}/include/platform/compress.h
            compress.cc)
target_link_libraries(cbcompress platform ${cbcompress_libraries})
generate_export_header(cbcompress
                       EXPORT_MACRO_NAME CBCOMPRESS_PUBLIC_API
                       EXPORT_FILE_NAME ${Platform_BINARY_DIR}/include/platform/compress-visibility.h)

install(TARGETS cbcompress
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

add_executable(platform-compression-test compression_test.cc)
target_link_libraries(platform-compression-test cbcompress gtest gtest_main)
add_test(platform-compression-test platform-compression-test)

INCLUDE_DIRECTORIES(AFTER ${benchmark_SOURCE_DIR}/include)
ADD_EXECUTABLE(platform-compression-bench
               compress_bench.cc)
TARGET_LINK_LIBRARIES(platform-compression-bench
                      ${cbcompress_libraries}
                      benchmark
                      gtest)

include_directories(AFTER ${SNAPPY_INCLUDE_DIR})
add_library(cbcompress SHARED
            ${Platform_SOURCE_DIR}/include/platform/compress.h
            compress.cc)
target_link_libraries(cbcompress platform ${SNAPPY_LIBRARIES})
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

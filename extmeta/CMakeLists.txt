add_library(extmeta SHARED ${Platform_SOURCE_DIR}/include/extmeta/extmeta.h extmeta.cc)
target_link_libraries(extmeta platform ${COUCHBASE_NETWORK_LIBS})
generate_export_header(extmeta
                       EXPORT_MACRO_NAME EXTMETA_PUBLIC_API
                       EXPORT_FILE_NAME ${Platform_BINARY_DIR}/include/extmeta/extmeta-visibility.h)

add_executable(platform_extmeta_test extmeta_test.cc)
target_link_libraries(platform_extmeta_test extmeta gtest gtest_main)
add_test(NAME platform-extmeta-test
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
         COMMAND platform_extmeta_test)

install(TARGETS extmeta
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

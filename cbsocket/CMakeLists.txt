include_directories(AFTER ${LIBEVENT_INCLUDE_DIR})
add_library(cbsocket SHARED
            ${Platform_SOURCE_DIR}/include/platform/socket.h
            socket.cc)

target_link_libraries(cbsocket
                      dirutils
                      gsl_lite
                      platform
                      ${LIBEVENT_LIBRARIES}
                      ${COUCHBASE_NETWORK_LIBS})

generate_export_header(cbsocket
                       EXPORT_MACRO_NAME CBSOCKET_PUBLIC_API
                       EXPORT_FILE_NAME ${Platform_BINARY_DIR}/include/platform/socket-visibility.h)

if (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
    add_executable(cbsocket_test cbsocket_test.cc)
    target_include_directories(cbsocket_test SYSTEM PRIVATE ${FOLLY_INCLUDE_DIR})
    target_include_directories(cbsocket_test PRIVATE ${Platform_SOURCE_DIR}/include)
    target_link_libraries(cbsocket_test cbsocket gtest gtest_main)
    add_test(cbsocket_test cbsocket_test)
endif()

install(TARGETS cbsocket
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

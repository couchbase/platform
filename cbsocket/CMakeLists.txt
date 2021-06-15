include_directories(AFTER ${LIBEVENT_INCLUDE_DIR})
add_library(cbsocket STATIC
            ${Platform_SOURCE_DIR}/include/platform/socket.h
            socket.cc)

target_link_libraries(cbsocket
                      gsl_lite
                      platform
                      ${LIBEVENT_LIBRARIES}
                      ${COUCHBASE_NETWORK_LIBS})
platform_enable_pch(cbsocket)

if (COUCHBASE_PLATFORM_BUILD_UNIT_TESTS)
    cb_add_test_executable(cbsocket_test cbsocket_test.cc)
    target_include_directories(cbsocket_test PRIVATE ${Platform_SOURCE_DIR}/include)
    target_link_libraries(cbsocket_test cbsocket gtest gtest_main)
    platform_enable_pch(cbsocket_test)
    add_test(cbsocket_test cbsocket_test)
endif()

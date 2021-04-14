ADD_EXECUTABLE(platform-backtrace-test backtrace_test.cc)
TARGET_LINK_LIBRARIES(platform-backtrace-test
    gtest
    gtest_main
    platform_cbassert
    platform_headers)
ADD_TEST(platform-backtrace-test platform-backtrace-test)

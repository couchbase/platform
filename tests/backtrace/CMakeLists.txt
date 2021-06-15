cb_add_test_executable(platform-backtrace-test backtrace_test.cc)
TARGET_LINK_LIBRARIES(platform-backtrace-test
    gtest
    gtest_main
    platform_cbassert
    platform_headers)
platform_enable_pch(platform-backtrace-test)
ADD_TEST(platform-backtrace-test platform-backtrace-test)

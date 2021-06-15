cb_add_test_executable(platform-getopt-test cb_getopt_test.cc getopt_test.cc)
TARGET_LINK_LIBRARIES(platform-getopt-test platform gtest gtest_main)
platform_enable_pch(platform-getopt-test)

ADD_TEST(platform-getopt-test platform-getopt-test)

cb_add_test_executable(platform-thread-test thread_test.cc)
TARGET_LINK_LIBRARIES(platform-thread-test platform gtest gtest_main)
platform_enable_pch(platform-thread-test)
ADD_TEST(platform-thread-test platform-thread-test)

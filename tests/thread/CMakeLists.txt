cb_add_test_executable(platform-thread-test thread_test.cc)
TARGET_LINK_LIBRARIES(platform-thread-test platform gtest gtest_main)
ADD_TEST(platform-thread-test platform-thread-test)

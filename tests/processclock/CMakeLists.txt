cb_add_test_executable(platform-processclock-test processclock_test.cc)
TARGET_LINK_LIBRARIES(platform-processclock-test gtest_main platform)
platform_enable_pch(platform-processclock-test)
ADD_TEST(platform-processclock-test platform-processclock-test)

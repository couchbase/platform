cb_add_test_executable(platform-processclock-test processclock_test.cc)
TARGET_LINK_LIBRARIES(platform-processclock-test gtest_main platform)
ADD_TEST(platform-processclock-test platform-processclock-test)

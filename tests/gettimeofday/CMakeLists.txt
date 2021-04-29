cb_add_test_executable(platform-gettimeofday-test gettimeofday_test.cc)
TARGET_LINK_LIBRARIES(platform-gettimeofday-test platform)
ADD_TEST(platform-gettimeofday-test platform-gettimeofday-test)

cb_add_test_executable(platform-gettimeofday-test gettimeofday_test.cc)
TARGET_LINK_LIBRARIES(platform-gettimeofday-test platform)
platform_enable_pch(platform-gettimeofday-test)
ADD_TEST(platform-gettimeofday-test platform-gettimeofday-test)

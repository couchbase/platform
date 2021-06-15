cb_add_test_executable(platform-timeutils-test timeutils_test.cc)
TARGET_LINK_LIBRARIES(platform-timeutils-test platform gtest gtest_main)
platform_enable_pch(platform-timeutils-test)
ADD_TEST(platform-timeutils-test platform-timeutils-test)

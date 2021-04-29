cb_add_test_executable(platform-timeutils-test timeutils_test.cc)
TARGET_LINK_LIBRARIES(platform-timeutils-test platform gtest gtest_main)
ADD_TEST(platform-timeutils-test platform-timeutils-test)

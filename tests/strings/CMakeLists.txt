cb_add_test_executable(platform-strings-test hex_test.cc)
TARGET_LINK_LIBRARIES(platform-strings-test platform gtest gtest_main)
platform_enable_pch(platform-strings-test)
ADD_TEST(platform-strings-test platform-strings-test)

cb_add_test_executable(platform-strings-test hex_test.cc)
TARGET_LINK_LIBRARIES(platform-strings-test platform gtest gtest_main)
ADD_TEST(platform-strings-test platform-strings-test)

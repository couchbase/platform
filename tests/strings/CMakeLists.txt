ADD_EXECUTABLE(platform-strings-test strings_test.c)
TARGET_LINK_LIBRARIES(platform-strings-test platform)
ADD_TEST(platform-strings-test platform-strings-test)

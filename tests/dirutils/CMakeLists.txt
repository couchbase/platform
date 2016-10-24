ADD_EXECUTABLE(platform-dirutils-test dirutils_test.cc)
TARGET_LINK_LIBRARIES(platform-dirutils-test dirutils gtest gtest_main)
ADD_TEST(platform-dirutils-test platform-dirutils-test)

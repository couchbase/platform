ADD_EXECUTABLE(platform-compression-test compression_test.cc)
TARGET_LINK_LIBRARIES(platform-compression-test cbcompress gtest gtest_main)
ADD_TEST(platform-compression-test platform-compression-test)

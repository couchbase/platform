ADD_EXECUTABLE(platform-n-byte-integer-test n_byte_integer_test.cc)
TARGET_LINK_LIBRARIES(platform-n-byte-integer-test gtest
                                                   gtest_main
                                                   platform_headers)
ADD_TEST(platform-n-byte-integer-test platform-n-byte-integer-test)
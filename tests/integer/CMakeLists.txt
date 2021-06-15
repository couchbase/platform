cb_add_test_executable(platform-n-byte-integer-test n_byte_integer_test.cc)
TARGET_LINK_LIBRARIES(platform-n-byte-integer-test gtest
                                                   gtest_main
                                                   platform_headers)
platform_enable_pch(platform-n-byte-integer-test)
ADD_TEST(platform-n-byte-integer-test platform-n-byte-integer-test)
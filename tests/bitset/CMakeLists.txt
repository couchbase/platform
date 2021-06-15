cb_add_test_executable(bitset-test bitset_test.cc)
TARGET_LINK_LIBRARIES(bitset-test gtest gmock gtest_main platform_headers)
platform_enable_pch(bitset-test)
ADD_TEST(bitset-test bitset-test)

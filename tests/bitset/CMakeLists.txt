ADD_EXECUTABLE(bitset-test bitset_test.cc)
TARGET_LINK_LIBRARIES(bitset-test gtest gmock gtest_main platform_headers ${FOLLY_LIBRARIES})
ADD_TEST(bitset-test bitset-test)

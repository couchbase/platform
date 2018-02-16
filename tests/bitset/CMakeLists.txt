ADD_EXECUTABLE(bitset-test bitset_test.cc)
TARGET_LINK_LIBRARIES(bitset-test gtest gmock gtest_main)
TARGET_INCLUDE_DIRECTORIES(bitset-test PRIVATE ${Platform_SOURCE_DIR}/include)
ADD_TEST(bitset-test bitset-test)

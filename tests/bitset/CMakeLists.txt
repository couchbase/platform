INCLUDE_DIRECTORIES(AFTER
        ${gtest_SOURCE_DIR}/include
        ${gmock_SOURCE_DIR}/include)
ADD_EXECUTABLE(bitset-test bitset_test.cc)
TARGET_LINK_LIBRARIES(bitset-test gtest gmock gtest_main)
ADD_TEST(bitset-test bitset-test)

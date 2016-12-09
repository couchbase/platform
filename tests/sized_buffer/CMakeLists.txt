INCLUDE_DIRECTORIES(AFTER
        ${gtest_SOURCE_DIR}/include
        ${gmock_SOURCE_DIR}/include)
ADD_EXECUTABLE(sized-buffer-test sized_buffer_test.cc)
TARGET_LINK_LIBRARIES(sized-buffer-test gtest gmock gtest_main)
ADD_TEST(sized-buffer-test sized-buffer-test)

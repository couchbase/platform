ADD_EXECUTABLE(sized-buffer-test sized_buffer_test.cc)
TARGET_LINK_LIBRARIES(sized-buffer-test platform gtest gmock gtest_main)
ADD_TEST(sized-buffer-test sized-buffer-test)

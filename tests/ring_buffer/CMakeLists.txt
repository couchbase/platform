cb_add_test_executable(ring-buffer-test ring_buffer_test.cc)
TARGET_LINK_LIBRARIES(ring-buffer-test gtest gtest_main platform_headers)
ADD_TEST(ring-buffer-test ring-buffer-test)

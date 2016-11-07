INCLUDE_DIRECTORIES(AFTER ${gtest_SOURCE_DIR}/include)
ADD_EXECUTABLE(ring-buffer-test ring_buffer_test.cc)
TARGET_LINK_LIBRARIES(ring-buffer-test gtest gtest_main)
ADD_TEST(ring-buffer-test ring-buffer-test)

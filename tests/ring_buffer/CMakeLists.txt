ADD_EXECUTABLE(ring-buffer-test ring_buffer_test.cc)
TARGET_LINK_LIBRARIES(ring-buffer-test gtest gtest_main)
TARGET_INCLUDE_DIRECTORIES(ring-buffer-test PRIVATE
        ${Platform_SOURCE_DIR}/include
        ${FOLLY_INCLUDE_DIR})
ADD_TEST(ring-buffer-test ring-buffer-test)

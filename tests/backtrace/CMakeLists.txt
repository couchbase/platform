ADD_EXECUTABLE(platform-backtrace-test backtrace_test.cc)
TARGET_INCLUDE_DIRECTORIES(platform-backtrace-test PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}/../../include
                           ${FOLLY_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(platform-backtrace-test gtest gtest_main platform_cbassert)
ADD_TEST(platform-backtrace-test platform-backtrace-test)

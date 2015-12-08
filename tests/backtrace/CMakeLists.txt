ADD_EXECUTABLE(platform-backtrace-test backtrace_test.c)
TARGET_LINK_LIBRARIES(platform-backtrace-test platform)
ADD_TEST(platform-backtrace-test platform-backtrace-test)

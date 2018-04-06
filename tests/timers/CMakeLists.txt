ADD_EXECUTABLE(platform-timers-test scope_timer_test.cc)
TARGET_LINK_LIBRARIES(platform-timers-test platform gtest gtest_main)
ADD_TEST(platform-timers-test platform-timers-test)

ADD_EXECUTABLE(platform-checked-snprintf-test checked_snprintf_test.cc)
TARGET_LINK_LIBRARIES(platform-checked-snprintf-test gtest gtest_main platform)
ADD_TEST(platform-checked-snprintf-test platform-checked-snprintf-test)

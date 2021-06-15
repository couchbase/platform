cb_add_test_executable(platform-checked-snprintf-test checked_snprintf_test.cc)
TARGET_LINK_LIBRARIES(platform-checked-snprintf-test gtest gtest_main platform)
platform_enable_pch(platform-checked-snprintf-test)
ADD_TEST(platform-checked-snprintf-test platform-checked-snprintf-test)

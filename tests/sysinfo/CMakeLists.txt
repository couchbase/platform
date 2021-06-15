cb_add_test_executable(platform-sysinfo-test sysinfo_test.cc)
TARGET_LINK_LIBRARIES(platform-sysinfo-test platform gtest gtest_main)
platform_enable_pch(platform-sysinfo-test)
ADD_TEST(platform-sysinfo-test platform-sysinfo-test)

ADD_EXECUTABLE(platform-sysinfo-test sysinfo_test.cc)
TARGET_LINK_LIBRARIES(platform-sysinfo-test platform gtest gtest_main)
ADD_TEST(platform-sysinfo-test platform-sysinfo-test)

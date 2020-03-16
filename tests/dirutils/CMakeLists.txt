add_executable(platform-dirutils-test dirutils_test.cc dirutils_test_2.cc)
target_link_libraries(platform-dirutils-test dirutils gtest gtest_main)
add_test(platform-dirutils-test platform-dirutils-test)

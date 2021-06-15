cb_add_test_executable(platform-optional-test optional_test.cc)
target_link_libraries(platform-optional-test gtest_main platform)
platform_enable_pch(platform-optional-test)

add_test(platform-optional-test platform-optional-test)

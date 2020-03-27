add_executable(platform-optional-test optional_test.cc)
target_link_libraries(platform-optional-test gtest_main platform)

add_test(platform-optional-test platform-optional-test)

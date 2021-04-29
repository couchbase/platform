cb_add_test_executable(platform-uuid-test uuid_test.cc)
target_link_libraries(platform-uuid-test platform gtest gtest_main)
add_test(platform-uuid-test platform-uuid-test)

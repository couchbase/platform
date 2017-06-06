add_executable(platform_pipe_test pipe_test.cc)
target_link_libraries(platform_pipe_test platform cJSON gtest gtest_main)
add_test(NAME platform-pipe_test COMMAND platform_pipe_test)

add_executable(platform_pipe_test pipe_test.cc)
target_link_libraries(platform_pipe_test platform cJSON gtest gtest_main)
add_test(NAME platform-pipe_test COMMAND platform_pipe_test)

add_executable(platform_pipe_benchmark pipe_benchmark.cc)
target_include_directories(platform_pipe_benchmark
    PRIVATE
    ${benchmark_SOURCE_DIR}/include)
target_link_libraries(platform_pipe_benchmark platform benchmark)
add_test(NAME platform_pipe_benchmark COMMAND platform_pipe_benchmark)

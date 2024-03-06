cb_add_test_executable(platform_benchmarks base64_test_bench.cc json_checker_bench.cc json_log_bench.cc)
target_link_libraries(platform_benchmarks PRIVATE
                      benchmark::benchmark GTest::gtest platform JSON_checker)
platform_enable_pch(platform_benchmarks)

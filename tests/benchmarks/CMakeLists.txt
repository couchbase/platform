cb_add_test_executable(platform_benchmarks
               base64_test_bench.cc
                       json_checker_bench.cc)
target_include_directories(platform_benchmarks
                           SYSTEM
                           PRIVATE
                           ${benchmark_SOURCE_DIR}/include)
target_link_libraries(platform_benchmarks PRIVATE benchmark gtest platform JSON_checker)
platform_enable_pch(platform_benchmarks)

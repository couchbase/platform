cb_add_test_executable(platform-json-checker-test json_checker_test.cc)
TARGET_LINK_LIBRARIES(platform-json-checker-test
        JSON_checker
        gtest
        gtest_main
        platform_headers)
ADD_TEST(platform-json-checker-test platform-json-checker-test)

cb_add_test_executable(platform-json-checker-bench
  json_checker_bench.cc)
TARGET_INCLUDE_DIRECTORIES(platform-json-checker-bench
    SYSTEM PRIVATE
    ${benchmark_SOURCE_DIR}/include)
TARGET_LINK_LIBRARIES(platform-json-checker-bench
  benchmark gtest platform JSON_checker)

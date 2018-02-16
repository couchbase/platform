ADD_EXECUTABLE(platform-json-checker-test json_checker_test.cc)
TARGET_LINK_LIBRARIES(platform-json-checker-test
        JSON_checker
        gtest
        gtest_main
        platform_headers)
ADD_TEST(platform-json-checker-test platform-json-checker-test)

ADD_EXECUTABLE(platform-json-checker-bench
  json_checker_bench.cc)
TARGET_LINK_LIBRARIES(platform-json-checker-bench
  benchmark gtest platform JSON_checker)

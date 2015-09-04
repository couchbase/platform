INCLUDE_DIRECTORIES(AFTER ${gtest_SOURCE_DIR}/include)
ADD_EXECUTABLE(platform-json-checker-test json_checker_test.cc)
TARGET_LINK_LIBRARIES(platform-json-checker-test JSON_checker gtest gtest_main)
ADD_TEST(platform-json-checker-test platform-json-checker-test)

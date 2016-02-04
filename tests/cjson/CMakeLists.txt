ADD_EXECUTABLE(platform-cjson-test
               ${Platform_SOURCE_DIR}/include/cJSON.h
               cjson_test.c)
TARGET_LINK_LIBRARIES(platform-cjson-test cJSON)
ADD_TEST(platform-cjson-test platform-cjson-test)

ADD_EXECUTABLE(platform-cjson-parse-test
               ${Platform_SOURCE_DIR}/include/cJSON.h
               cjson_parse_test.c)

TARGET_LINK_LIBRARIES(platform-cjson-parse-test cJSON platform)
ADD_TEST(platform-cjson-parse-test
         platform-cjson-parse-test -f ${CMAKE_CURRENT_SOURCE_DIR}/testdata.json
         -n 1)

ADD_EXECUTABLE(platform-cjsonutils-test cjson_utils_test.cc)
TARGET_LINK_LIBRARIES(platform-cjsonutils-test cJSON gtest gtest_main)
ADD_TEST(platform-cjsonutils-test platform-cjsonutils-test)

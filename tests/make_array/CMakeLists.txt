ADD_EXECUTABLE(platform-make_array-test
               ${Platform_SOURCE_DIR}/include/platform/make_array.h
               ${Platform_SOURCE_DIR}/include/platform/platform.h
               make_array_test.cc)
TARGET_LINK_LIBRARIES(platform-make_array-test platform gtest gtest_main)
ADD_TEST(platform-make_array-test platform-make_array-test)

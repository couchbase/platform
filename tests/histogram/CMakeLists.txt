ADD_EXECUTABLE(platform-histogram-test
               ${Platform_SOURCE_DIR}/include/platform/histogram.h
               ${Platform_SOURCE_DIR}/include/platform/platform.h
               histogram_test.cc)
TARGET_LINK_LIBRARIES(platform-histogram-test platform gtest gtest_main)
ADD_TEST(platform-histogram-test platform-histogram-test)

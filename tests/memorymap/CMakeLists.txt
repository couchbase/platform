ADD_EXECUTABLE(platform-memorymap-test
               ${Platform_SOURCE_DIR}/include/platform/memorymap.h
               memorymap_test.cc)
TARGET_LINK_LIBRARIES(platform-memorymap-test platform dirutils)
ADD_TEST(platform-memorymap-test platform-memorymap-test)

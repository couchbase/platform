cb_add_test_executable(platform-random-test
               ${Platform_SOURCE_DIR}/include/platform/random.h
               random_test.cc)
TARGET_LINK_LIBRARIES(platform-random-test platform)
ADD_TEST(platform-random-test platform-random-test)

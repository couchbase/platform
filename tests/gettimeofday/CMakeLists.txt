ADD_EXECUTABLE(platform-gettimeofday-test gettimeofday_test.cc)
TARGET_LINK_LIBRARIES(platform-gettimeofday-test platform ${FOLLY_LIBRARIES})
ADD_TEST(platform-gettimeofday-test platform-gettimeofday-test)

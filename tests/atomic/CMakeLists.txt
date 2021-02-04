ADD_EXECUTABLE(platform-relaxed_atomic-test relaxed_atomic_test.cc)
TARGET_LINK_LIBRARIES(platform-relaxed_atomic-test PRIVATE
        gtest
        gtest_main
        platform_headers)
ADD_TEST(platform-relaxed_atomic-test platform-relaxed_atomic-test)

ADD_EXECUTABLE(platform-non_negative_counter-test non_negative_counter_test.cc)
TARGET_LINK_LIBRARIES(platform-non_negative_counter-test PRIVATE
        gtest
        gtest_main
        platform)
ADD_TEST(platform-non_negative_counter-test platform-non_negative_counter-test)

ADD_EXECUTABLE(platform-atomic_duration-test atomic_duration_test.cc)
TARGET_LINK_LIBRARIES(platform-atomic_duration-test PRIVATE
        gtest
        gtest_main
        platform_headers
        ${FOLLY_LIBRARIES})
ADD_TEST(platform-atomic_duration-test platform-atomic_duration-test)

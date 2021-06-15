cb_add_test_executable(platform-relaxed_atomic-test relaxed_atomic_test.cc)
TARGET_LINK_LIBRARIES(platform-relaxed_atomic-test PRIVATE
        gtest
        gtest_main
        platform_headers)
platform_enable_pch(platform-relaxed_atomic-test)
ADD_TEST(platform-relaxed_atomic-test platform-relaxed_atomic-test)

cb_add_test_executable(platform-non_negative_counter-test non_negative_counter_test.cc)
TARGET_LINK_LIBRARIES(platform-non_negative_counter-test PRIVATE
        gtest
        gtest_main
        platform)
platform_enable_pch(platform-non_negative_counter-test)
ADD_TEST(platform-non_negative_counter-test platform-non_negative_counter-test)

cb_add_test_executable(platform-atomic_duration-test atomic_duration_test.cc)
TARGET_LINK_LIBRARIES(platform-atomic_duration-test PRIVATE
        gtest
        gtest_main
        platform_headers)
platform_enable_pch(platform-atomic_duration-test)
ADD_TEST(platform-atomic_duration-test platform-atomic_duration-test)

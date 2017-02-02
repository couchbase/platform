ADD_EXECUTABLE(platform-relaxed_atomic-test relaxed_atomic_test.cc)
TARGET_LINK_LIBRARIES(platform-relaxed_atomic-test gtest gtest_main)
ADD_TEST(platform-relaxed_atomic-test platform-relaxed_atomic-test)


ADD_EXECUTABLE(platform-non_negative_counter-test non_negative_counter_test.cc)
TARGET_LINK_LIBRARIES(platform-non_negative_counter-test gtest gtest_main)
ADD_TEST(platform-non_negative_counter-test platform-non_negative_counter-test)

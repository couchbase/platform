ADD_EXECUTABLE(platform-relaxed_atomic-test relaxed_atomic_test.cc)
TARGET_LINK_LIBRARIES(platform-relaxed_atomic-test gtest gtest_main)
ADD_TEST(platform-relaxed_atomic-test platform-relaxed_atomic-test)

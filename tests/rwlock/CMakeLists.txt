cb_add_test_executable(rwlock_test rwlock_test.cc)
TARGET_LINK_LIBRARIES(rwlock_test PRIVATE GTest::gtest GTest::gtest_main platform)
platform_enable_pch(rwlock_test)
ADD_TEST(rwlock_test rwlock_test)

# The death tests in this suite rely on halting as soon as an error is detected.
# For macOS must explicitly enable deadlock detection.
SET_TESTS_PROPERTIES(rwlock_test PROPERTIES
  ENVIRONMENT "TSAN_OPTIONS=halt_on_error=1 detect_deadlocks=1")

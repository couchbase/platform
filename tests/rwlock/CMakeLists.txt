cb_add_test_executable(rwlock_test rwlock_test.cc)
target_link_libraries(rwlock_test PRIVATE GTest::gtest GTest::gtest_main platform)
platform_enable_pch(rwlock_test)
add_test(rwlock_test rwlock_test)

# The death tests in this suite rely on halting as soon as an error is detected.
# For macOS must explicitly enable deadlock detection.
set_tests_properties(rwlock_test PROPERTIES
  ENVIRONMENT "TSAN_OPTIONS=halt_on_error=1 detect_deadlocks=1")

cb_add_test_executable(platform-semaphore-test semaphore_test.cc)
target_link_libraries(platform-semaphore-test gtest_main platform)
platform_enable_pch(platform-semaphore-test)

add_test(platform-semaphore-test platform-semaphore-test)

add_executable(platform-semaphore-test semaphore_test.cc)
target_link_libraries(platform-semaphore-test gtest_main platform)

add_test(platform-semaphore-test platform-semaphore-test)

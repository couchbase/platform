ADD_SUBDIRECTORY(json_checker)

INCLUDE_DIRECTORIES(AFTER ${gtest_SOURCE_DIR}/include)
ADD_EXECUTABLE(platform-relaxed_atomic-test relaxed_atomic_test.cc)
TARGET_LINK_LIBRARIES(platform-relaxed_atomic-test gtest gtest_main)
ADD_TEST(platform-relaxed_atomic-test platform-relaxed_atomic-test)

ADD_EXECUTABLE(platform-threadname-test threadname_test.cc)
TARGET_LINK_LIBRARIES(platform-threadname-test platform gtest gtest_main)
ADD_TEST(platform-threadname-test platform-threadname-test)

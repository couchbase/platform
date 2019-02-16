add_executable(platform-dirutils-test dirutils_test.cc)
target_link_libraries(platform-dirutils-test dirutils gtest gtest_main)
add_test(platform-dirutils-test platform-dirutils-test)

add_library(platform-dirutils-test-library SHARED dirutils-test-library.cc)
target_include_directories(platform-dirutils-test-library PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(platform-dirutils-test-library PROPERTIES PREFIX "")
generate_export_header(platform-dirutils-test-library)
add_dependencies(platform-dirutils-test platform-dirutils-test-library)

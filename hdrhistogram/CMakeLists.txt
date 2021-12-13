add_library(hdrhistogram STATIC
        ${Platform_SOURCE_DIR}/include/hdrhistogram/hdrhistogram.h
        ${Platform_SOURCE_DIR}/include/hdrhistogram/iterator_range.h
        hdrhistogram.cc)

target_include_directories(hdrhistogram PUBLIC ${Platform_SOURCE_DIR}/include)
# Mark hdr_histogram as 'system' so we skip any warnings it generates.
target_include_directories(hdrhistogram SYSTEM BEFORE PUBLIC
        ${hdr_histogram_SOURCE_DIR}/src)
target_link_libraries(hdrhistogram PRIVATE fmt::fmt hdr_histogram_static
        Folly::folly platform nlohmann_json::nlohmann_json)

set_property(TARGET hdrhistogram PROPERTY POSITION_INDEPENDENT_CODE true)

cb_add_test_executable(hdrhistogram_test hdrhistogram_test.cc)
target_link_libraries(hdrhistogram_test hdrhistogram
        fmt::fmt
        nlohmann_json::nlohmann_json
        ${Boost_LIBRARIES}
        Folly::folly
        GTest::gtest
        GTest::gtest_main)
add_test(hdrhistogram_test hdrhistogram_test)

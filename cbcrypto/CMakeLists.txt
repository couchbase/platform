add_library(cbcrypto STATIC
        common.cc
        digest.cc
        dump_keys_runner.cc
        encrypted_file_associated_data.h
        encrypted_file_header.cc
        file_reader.cc
        file_utilities.cc
        file_writer.cc
        key_store.cc
        random_gen.cc
        symmetric.cc
        ${Platform_SOURCE_DIR}/include/cbcrypto/common.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/digest.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/dump_keys_runner.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/encrypted_file_header.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/file_reader.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/file_utilities.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/file_writer.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/key_store.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/random_gen.h
        ${Platform_SOURCE_DIR}/include/cbcrypto/symmetric.h
)
set_property(TARGET cbcrypto PROPERTY POSITION_INDEPENDENT_CODE 1)
platform_enable_pch(cbcrypto)
target_link_libraries(cbcrypto
        PUBLIC
        gsl::gsl-lite
        nlohmann_json::nlohmann_json
        PRIVATE
        cbcompress
        fmt::fmt
        libsodium::libsodium
        OpenSSL::SSL
        phosphor
        platform
        ZLIB::ZLIB)

cb_add_test_executable(cbcrypto_unit_test
        cbcrypto_test.cc
        file_utilities_test.cc
        fileio_test.cc
        key_store_test.cc
)
cb_enable_unity_build(cbcrypto_unit_test)
platform_enable_pch(cbcrypto_unit_test)
target_link_libraries(cbcrypto_unit_test
                     PRIVATE
                     cbcrypto
                     GTest::gtest
                     GTest::gtest_main
                     platform)
add_test(cbcrypto_unit_test cbcrypto_unit_test)

add_executable(cbcat cbcat.cc)
target_compile_definitions(cbcat
        PRIVATE DESTINATION_ROOT="${CMAKE_INSTALL_PREFIX}"
        PRODUCT_VERSION="${PRODUCT_VERSION}")
target_link_libraries(cbcat
        PRIVATE
        cbcrypto
        platform
        ${Boost_LIBRARIES})
install(TARGETS cbcat RUNTIME DESTINATION bin)

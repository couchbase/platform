include_directories(AFTER ${LIBEVENT_INCLUDE_DIR})
add_library(cbsocket SHARED
            ${Platform_SOURCE_DIR}/include/platform/socket.h
            socket.cc)

target_link_libraries(cbsocket
                      dirutils
                      platform
                      ${LIBEVENT_LIBRARIES}
                      ${COUCHBASE_NETWORK_LIBS})

generate_export_header(cbsocket
                       EXPORT_MACRO_NAME CBSOCKET_PUBLIC_API
                       EXPORT_FILE_NAME ${Platform_BINARY_DIR}/include/platform/socket-visibility.h)

install(TARGETS cbsocket
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

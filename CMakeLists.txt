PROJECT(Platform)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX("chrono" HAVE_CHRONO)
CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/src/config.cmake.h
                ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

#
# Set the include path
#
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
                           ${CMAKE_CURRENT_BINARY_DIR}/src)

ADD_LIBRARY(cJSON SHARED src/cJSON.c include/cJSON.h)
SET_TARGET_PROPERTIES(cJSON PROPERTIES SOVERSION 1.0.0)
SET_TARGET_PROPERTIES(cJSON PROPERTIES COMPILE_FLAGS -DBUILDING_CJSON=1)
SET_TARGET_PROPERTIES(cJSON PROPERTIES INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/lib)

IF (WIN32)
   INCLUDE_DIRECTORIES(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include/win32)
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
   SET(PLATFORM_FILES src/getopt.c
                      src/sockets.c
                      src/cb_win32.c
                      src/winrandom.c
                      include/win32/getopt.h
                      include/win32/strings.h
                      include/win32/unistd.h)
ELSE (WIN32)
   SET(PLATFORM_FILES src/cb_pthreads.c src/urandom.c)
   SET(THREAD_LIBS "pthread")
   SET(DLOPENLIB "dl")

   IF (NOT APPLE)
      SET(RTLIB "rt")
   ENDIF(NOT APPLE)

   TARGET_LINK_LIBRARIES(cJSON m)
ENDIF (WIN32)

IF (NOT DEFINED COUCHBASE_NETWORK_LIBS)
   IF (WIN32)
      SET(COUCHBASE_NETWORK_LIBS "Ws2_32")
   ELSEIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
      SET(COUCHBASE_NETWORK_LIBS socket nsl)
   ENDIF (WIN32)
ENDIF (NOT DEFINED COUCHBASE_NETWORK_LIBS)

IF(HAVE_CHRONO)
   SET(PLATFORM_FILES ${PLATFORM_FILES} src/cxxtime.cc)
ELSE(HAVE_CHRONO)
   SET(PLATFORM_FILES ${PLATFORM_FILES} src/gethrtime.c)
ENDIF(HAVE_CHRONO)

#
# Add all of the libraries
#
ADD_LIBRARY(platform SHARED ${PLATFORM_FILES}
                            ${CMAKE_CURRENT_BINARY_DIR}/src/config.h
                            src/random.cc
                            src/byteorder.c
                            include/platform/platform.h
                            include/platform/random.h
                            include/platform/visibility.h)

TARGET_LINK_LIBRARIES(platform ${THREAD_LIBS} ${COUCHBASE_NETWORK_LIBS} ${DLOPENLIB} ${RTLIB})
SET_TARGET_PROPERTIES(platform PROPERTIES SOVERSION 0.1.0)
SET_TARGET_PROPERTIES(platform PROPERTIES INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/lib)

ADD_LIBRARY(dirutils SHARED src/dirutils.cc include/platform/dirutils.h)
SET_TARGET_PROPERTIES(dirutils PROPERTIES SOVERSION 0.1.0)
SET_TARGET_PROPERTIES(dirutils PROPERTIES INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/lib)

ADD_EXECUTABLE(platform-dirutils-test tests/dirutils_test.cc)
TARGET_LINK_LIBRARIES(platform-dirutils-test dirutils)

ADD_EXECUTABLE(platform-gettimeofday-test tests/gettimeofday_test.cc)
TARGET_LINK_LIBRARIES(platform-gettimeofday-test platform)

IF (INSTALL_HEADER_FILES)
   INSTALL (FILES
            include/platform/cbassert.h
            include/platform/dirutils.h
            include/platform/platform.h
            include/platform/random.h
            include/platform/visibility.h
            include/platform/dirutils.h
            DESTINATION include/platform)

   INSTALL(FILES include/cJSON.h DESTINATION include)

   IF (WIN32)
      INSTALL(FILES
             include/win32/getopt.h
             include/win32/strings.h
             include/win32/unistd.h
             DESTINATION include)
   ENDIF (WIN32)
ENDIF (INSTALL_HEADER_FILES)

INSTALL(TARGETS cJSON platform dirutils
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

ADD_CUSTOM_TARGET(reformat-platform-source
                  COMMAND astyle ${ASTYLE_CFLAGS}
                  VERBATIM)

#
# Test targets
#
ADD_EXECUTABLE(platform-gethrtime-test
               tests/gethrtime_test.c
               include/platform/platform.h)
TARGET_LINK_LIBRARIES(platform-gethrtime-test platform)

ADD_EXECUTABLE(platform-getopt-test tests/getopt_test.cc)
TARGET_LINK_LIBRARIES(platform-getopt-test platform)

ADD_EXECUTABLE(platform-random-test
               tests/random_test.cc
               include/platform/random.h)
TARGET_LINK_LIBRARIES(platform-random-test platform)

ADD_TEST(platform-dirutils-test platform-dirutils-test)
ADD_TEST(platform-gettimeofday-test platform-gettimeofday-test)
ADD_TEST(platform-gethrtime-test platform-gethrtime-test)
ADD_TEST(platform-getopt-test-0 platform-getopt-test 0)
ADD_TEST(platform-getopt-test-1 platform-getopt-test 1)
ADD_TEST(platform-getopt-test-2 platform-getopt-test 2)
ADD_TEST(platform-random-test platform-random-test)

ADD_EXECUTABLE(platform_cjson_test include/cJSON.h tests/cjson_test.c)
TARGET_LINK_LIBRARIES(platform_cjson_test cJSON)
ADD_TEST(platform-cjson-test platform_cjson_test)

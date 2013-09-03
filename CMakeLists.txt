CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#
# Set the include path
#
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include)

IF (WIN32)
   SET(PLATFORM_FILES src/getopt.c
                      src/sockets.c
                      src/cb_win32.c)
ELSE (WIN32)
   SET(PLATFORM_FILES src/cb_pthreads.c)
   SET(MATH_LIBS "m")
   SET(THREAD_LIBS "pthread")
   SET(DLOPENLIB "dl")

IF (NOT APPLE)
   SET(RTLIB "rt")
ENDIF(NOT APPLE)

ENDIF (WIN32)

ADD_DEFINITIONS(-DBUILDING_LIBPLATFORM=1)

#
# Add all of the libraries
#
ADD_LIBRARY(platform SHARED ${PLATFORM_FILES} src/gethrtime.c)
TARGET_LINK_LIBRARIES(platform ${THREAD_LIBS} ${COUCHBASE_NETWORK_LIBS} ${DLOPENLIB} ${RTLIB})
SET_TARGET_PROPERTIES(platform PROPERTIES SOVERSION 0.1.0)


INSTALL (FILES
         include/platform/platform.h
         include/platform/visibility.h
         DESTINATION include/platform)

IF (WIN32)
INSTALL (FILES
         src/getopt.h
         DESTINATION include)
ENDIF (WIN32)

INSTALL(TARGETS platform
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

ADD_CUSTOM_TARGET(reformat-platform-source
                  COMMAND astyle ${ASTYLE_CFLAGS}
                  VERBATIM)

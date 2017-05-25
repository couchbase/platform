PROJECT(Platform)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF (${CMAKE_MAJOR_VERSION} GREATER 2)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF (${CMAKE_MAJOR_VERSION} GREATER 2)


INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckSymbolExists)
INCLUDE(CTest)
INCLUDE(GenerateExportHeader)

ENABLE_CODE_COVERAGE_REPORT()

CMAKE_PUSH_CHECK_STATE(RESET)
  FIND_LIBRARY(EXECINFO_LIBRARY NAMES execinfo)
  IF (EXECINFO_LIBRARY)
    SET(CMAKE_REQUIRED_LIBRARIES "${EXECINFO_LIBRARY}")
    LIST(APPEND PLATFORM_LIBRARIES "${EXECINFO_LIBRARY}")
  ENDIF(EXECINFO_LIBRARY)
  CHECK_SYMBOL_EXISTS(backtrace execinfo.h HAVE_BACKTRACE)
CMAKE_POP_CHECK_STATE()

CMAKE_PUSH_CHECK_STATE(RESET)
  SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
  FIND_LIBRARY(DL_LIBRARY NAMES dl)
  IF (DL_LIBRARY)
    SET(CMAKE_REQUIRED_LIBRARIES "${DL_LIBRARY}")
    LIST(APPEND PLATFORM_LIBRARIES "${DL_LIBRARY}")
  ENDIF (DL_LIBRARY)
  CHECK_SYMBOL_EXISTS(dladdr dlfcn.h HAVE_DLADDR)
CMAKE_POP_CHECK_STATE()

CMAKE_PUSH_CHECK_STATE(RESET)
SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
SET(CMAKE_REQUIRED_LIBRARIES "pthread")
CHECK_SYMBOL_EXISTS(pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
CHECK_SYMBOL_EXISTS(pthread_getname_np pthread.h HAVE_PTHREAD_GETNAME_NP)
CHECK_SYMBOL_EXISTS(sched_getaffinity sched.h HAVE_SCHED_GETAFFINITY)
CMAKE_POP_CHECK_STATE()

CMAKE_PUSH_CHECK_STATE(RESET)
CHECK_CXX_SOURCE_COMPILES("void f() noexcept; int main() { return 0; }" HAVE_NOEXCEPT)

CHECK_CXX_SOURCE_COMPILES("
    #include <memory>
    int main() {
        return *std::make_unique<int>(0);
    }" HAVE_MAKE_UNIQUE)
CMAKE_POP_CHECK_STATE()

CHECK_SYMBOL_EXISTS(gethrtime sys/time.h CB_DONT_NEED_GETHRTIME)
CHECK_SYMBOL_EXISTS(htonll arpa/inet.h CB_DONT_NEED_BYTEORDER)

CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/src/config.cmake.h
                ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/include/platform/dynamic.in.h
                ${CMAKE_CURRENT_BINARY_DIR}/include/platform/dynamic.h)

#
# Set the include path
#
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
                           ${CMAKE_CURRENT_BINARY_DIR}/src
                           ${phosphor_SOURCE_DIR}/include)

IF (MEMORY_ALLOCATOR)
  INCLUDE_DIRECTORIES(AFTER ${MALLOC_INCLUDE_DIR})
ENDIF (MEMORY_ALLOCATOR)

ADD_LIBRARY(cJSON SHARED
            include/cJSON.h
            include/cJSON_utils.h
            src/cJSON.cc
            src/cJSON_utils.cc)
SET_TARGET_PROPERTIES(cJSON PROPERTIES SOVERSION 1.1.0)
SET_TARGET_PROPERTIES(cJSON PROPERTIES COMPILE_FLAGS -DBUILDING_CJSON=1)

ADD_LIBRARY(JSON_checker SHARED src/JSON_checker.cc include/JSON_checker.h)
SET_TARGET_PROPERTIES(JSON_checker PROPERTIES SOVERSION 1.0.0)

IF (WIN32)
   INCLUDE_DIRECTORIES(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include/win32)
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
   SET(PLATFORM_FILES src/getopt.c
                      src/sockets.c
                      src/cb_win32.cc
                      src/winrandom.c
                      src/memorymap_win32.cc
                      include/win32/getopt.h
                      include/win32/strings.h
                      include/win32/unistd.h)

   # Allow to build on unsupported platorms (currently with MSVC 2017CE)
   # and the FindCouchbaseDbgHelp macro didn't pick it up in the new location
   # stuff has been moved around for the CE version. The library is only used
   # for backtrace support which we can live without
   get_supported_production_platform(_supported_platform)
   if (_supported_platform)
       ADD_DEFINITIONS(-DHAVE_BACKTRACE_SUPPORT=1)
       INCLUDE(FindCouchbaseDbgHelp)
       LIST(APPEND PLATFORM_LIBRARIES "${DBGHELP_LIBRARY}")
       INSTALL(FILES ${DBGHELP_DLL} DESTINATION bin)
   endif ()
ELSE (WIN32)
   SET(PLATFORM_FILES src/cb_pthreads.cc src/urandom.c src/memorymap_posix.cc)
   SET_SOURCE_FILES_PROPERTIES(src/crc32c_sse4_2.cc PROPERTIES COMPILE_FLAGS -msse4.2)
   LIST(APPEND PLATFORM_LIBRARIES "pthread")

   IF (NOT APPLE)
      LIST(APPEND PLATFORM_LIBRARIES "rt")
   ENDIF(NOT APPLE)

   SET(LIB_M m)
ENDIF (WIN32)

TARGET_LINK_LIBRARIES(cJSON platform ${LIB_M})

IF (NOT DEFINED COUCHBASE_NETWORK_LIBS)
   IF (WIN32)
      SET(COUCHBASE_NETWORK_LIBS "Ws2_32")
   ELSEIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
      SET(COUCHBASE_NETWORK_LIBS socket nsl)
   ENDIF (WIN32)
ENDIF (NOT DEFINED COUCHBASE_NETWORK_LIBS)

IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
    # The implementation you get with chrono on a default FreeBSD 10
    # is dead slow and considered useless...
    SET(HAVE_DEAD_SLOW_CHRONO true)
ENDIF ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")

IF (CB_DONT_NEED_GETHRTIME OR HAVE_DEAD_SLOW_CHRONO)
   SET(PLATFORM_FILES ${PLATFORM_FILES} src/gethrtime.c)
ELSE (CB_DONT_NEED_GETHRTIME OR HAVE_DEAD_SLOW_CHRONO)
   CHECK_INCLUDE_FILE_CXX(chrono HAVE_CHRONO)
   IF (NOT HAVE_CHRONO)
      MESSAGE(FATAL_ERROR "You need chrono to build a high resolution timer")
   ENDIF (NOT HAVE_CHRONO)
   SET(PLATFORM_FILES ${PLATFORM_FILES} src/cxxtime.cc)
ENDIF (CB_DONT_NEED_GETHRTIME OR HAVE_DEAD_SLOW_CHRONO)

#
# Add all of the libraries
#

# platform_so - the 'real' platform shared library containing our various
# platform abstraction functionality.
# Note that in general external users should link 'platform' and not
# 'platform_so' - see below.
ADD_LIBRARY(platform_so SHARED ${PLATFORM_FILES}
                            ${CMAKE_CURRENT_BINARY_DIR}/src/config.h
                            src/base64.cc
                            src/getpid.c
                            src/random.cc
                            src/backtrace.c
                            src/byte_buffer_dump.cc
                            src/byteorder.c
                            src/cb_malloc.cc
                            src/cb_mktemp.c
                            src/cb_time.cc
                            src/cbassert.c
                            src/checked_snprintf.cc
                            src/crc32c.cc
                            src/crc32c_sse4_2.cc
                            src/crc32c_private.h
                            src/global_new_replacement.cc
                            src/processclock.cc
                            src/strerror.cc
                            src/string.cc
                            src/strnstr.cc
                            src/sysinfo.cc
                            src/thread.cc
                            src/timeutils.cc
                            src/uuid.cc
                            include/platform/base64.h
                            include/platform/byte_buffer_dump.h
                            include/platform/crc32c.h
                            include/platform/memorymap.h
                            include/platform/platform.h
                            include/platform/processclock.h
                            include/platform/random.h
                            include/platform/ring_buffer.h
                            include/platform/sized_buffer.h
                            include/platform/strerror.h
                            include/platform/string.h
                            include/platform/sysinfo.h
                            include/platform/thread.h
                            include/platform/timeutils.h
                            include/platform/visibility.h)

LIST(APPEND PLATFORM_LIBRARIES "phosphor")
LIST(REMOVE_DUPLICATES PLATFORM_LIBRARIES)
TARGET_LINK_LIBRARIES(platform_so
                      ${COUCHBASE_NETWORK_LIBS} ${MALLOC_LIBRARIES} ${PLATFORM_LIBRARIES})

# 'platform' - the library clients should normally link to.
#
# 'platform' is a static library, which itself links to the main platform_so
# shared library. This is to ensure that any clients of platform always link
# in the C++ global operator new/delete replacements if they use platform at
# all. If they didn't then this would cause problems:
# 1) Memory usage from the client will not be tracked.
# 2) If a C++ object allocated in platform (using cbmalloc via new) is
#    delete'd in the client (using default free) then we have a mismatched
#    new/delete pair.
# By giving all clients of platform the global new/delete automatically we
# avoid these problems.
ADD_LIBRARY(platform STATIC src/global_new_replacement.cc)
SET_TARGET_PROPERTIES(platform PROPERTIES POSITION_INDEPENDENT_CODE true)
TARGET_LINK_LIBRARIES(platform platform_so)


# On OS X 10.11 we need to remove the default rpath settings in order to all
# us to load the platform library from Python scripts that are run out of
# /usr/bin/python. We can remove this once we ship our own Python.
IF (APPLE)
    SET_TARGET_PROPERTIES(platform_so
                          PROPERTIES
                          INSTALL_RPATH "")
ENDIF ()

SET_TARGET_PROPERTIES(platform_so PROPERTIES SOVERSION 0.1.0)

# TODO: Given dirutils now links with platform (for cbmalloc &
# global_new_replacement) we can probably just merge it into platform.
ADD_LIBRARY(dirutils SHARED
            src/dirutils.cc
            include/platform/dirutils.h)
SET_TARGET_PROPERTIES(dirutils PROPERTIES SOVERSION 0.1.0)
TARGET_LINK_LIBRARIES(dirutils platform)
GENERATE_EXPORT_HEADER(dirutils
                       EXPORT_MACRO_NAME DIRUTILS_PUBLIC_API
                       EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/platform/dirutils-visibility.h)

IF (BREAKPAD_FOUND)
   ADD_DEFINITIONS(-DHAVE_BREAKPAD)
   INCLUDE_DIRECTORIES(AFTER ${BREAKPAD_INCLUDE_DIR})
   # Breakpad has headers (which are dependencies for our breakpad integration
   # code) that don't compile cleanly as C++11, so the GNU standard is
   # used instead.
   IF (UNIX)
      set_source_files_properties(src/breakpad_wrapper.cc PROPERTIES COMPILE_FLAGS ${CB_GNU_CXX11_OPTION})
   ENDIF (UNIX)
ENDIF (BREAKPAD_FOUND)

# To simplify usage we always build breakpad_wrapper; it'll just
# do nothing on targets which don't have Breakpad.
ADD_LIBRARY(breakpad_wrapper SHARED
            src/breakpad_wrapper.cc
            include/breakpad_wrapper/breakpad_wrapper.h)

TARGET_LINK_LIBRARIES(breakpad_wrapper
                      ${BREAKPAD_LIBRARIES})


INSTALL(TARGETS breakpad_wrapper cJSON JSON_checker platform_so dirutils
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

ADD_SUBDIRECTORY(extmeta)
ADD_SUBDIRECTORY(cbcompress)
ADD_SUBDIRECTORY(cbsocket)

#
# Test targets
#
ADD_SUBDIRECTORY(tests)

IF (${CMAKE_MAJOR_VERSION} LESS 3)
   SET_TARGET_PROPERTIES(cJSON PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
   SET_TARGET_PROPERTIES(JSON_checker PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
   SET_TARGET_PROPERTIES(platform_so PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
   SET_TARGET_PROPERTIES(dirutils PROPERTIES INSTALL_NAME_DIR
                         ${CMAKE_INSTALL_PREFIX}/lib)
ENDIF (${CMAKE_MAJOR_VERSION} LESS 3)

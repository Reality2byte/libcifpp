# SPDX-License-Identifier: BSD-2-Clause
# 
# Copyright (c) 2025 Maarten L. Hekkelman
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# A simplified wrapper CMakeLists.txt file for PCRE2
#
# We will only build a static 8bit library, no fancy switches at all

cmake_minimum_required(VERSION 3.25)

include(FetchContent)

project(pcre2s VERSION 1.0.0 LANGUAGES C CXX)

# The original code:

file(DOWNLOAD https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.46/pcre2-10.46.tar.gz
    ${CMAKE_CURRENT_BINARY_DIR}/pcre2-code.tgz
    EXPECTED_HASH SHA256=8d28d7f2c3b970c3a4bf3776bcbb5adfc923183ce74bc8df1ebaad8c1985bd07)
file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/pcre2-code.tgz
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(PCRE2_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/pcre2-10.46)
set(PCRE2_MAJOR 10)
set(PCRE2_MINOR 46)
set(PCRE2_VERSION "${PCRE2_MAJOR}.${PCRE2_MINOR}")
set(PCRE2_DATE "2024-06-09")

# Some needed configuration options

# option(PCRE2_BUILD_PCRE2_8 "Build 8 bit PCRE2 library" ON)
# option(PCRE2_BUILD_PCRE2_16 "Build 16 bit PCRE2 library" OFF)
# option(PCRE2_BUILD_PCRE2_32 "Build 32 bit PCRE2 library" OFF)

option(PCRE2_STATIC_PIC "Build the static library with the option position independent code enabled." OFF)

set(PCRE2_NEWLINE "LF" CACHE STRING "What to recognize as a newline (one of CR, LF, CRLF, ANY, ANYCRLF, NUL)." FORCE)
set_property(CACHE PCRE2_NEWLINE PROPERTY STRINGS "CR" "LF" "CRLF" "ANY" "ANYCRLF" "NUL")

set(PCRE2_LINK_SIZE "2" CACHE STRING "Internal link size (2, 3 or 4 allowed). See LINK_SIZE in config.h.in for details.")
set_property(CACHE PCRE2_LINK_SIZE PROPERTY STRINGS "2" "3" "4")

set(PCRE2_PARENS_NEST_LIMIT "250" CACHE STRING "Default nested parentheses limit. See PARENS_NEST_LIMIT in config.h.in for details.")
set(PCRE2_HEAP_LIMIT "20000000" CACHE STRING "Default limit on heap memory (kibibytes). See HEAP_LIMIT in config.h.in for details.")
set(PCRE2_MAX_VARLOOKBEHIND "255" CACHE STRING "Default limit on variable lookbehinds.")
set(PCRE2_MATCH_LIMIT "10000000" CACHE STRING "Default limit on internal looping. See MATCH_LIMIT in config.h.in for details.")
set(PCRE2_MATCH_LIMIT_DEPTH "MATCH_LIMIT" CACHE STRING "Default limit on internal depth of search. See MATCH_LIMIT_DEPTH in config.h.in for details.")
set(PCRE2GREP_BUFSIZE "20480" CACHE STRING "Buffer starting size parameter for pcre2grep. See PCRE2GREP_BUFSIZE in config.h.in for details.")
set(PCRE2GREP_MAX_BUFSIZE "1048576" CACHE STRING "Buffer maximum size parameter for pcre2grep. See PCRE2GREP_MAX_BUFSIZE in config.h.in for details.")
set(PCRE2_SUPPORT_JIT OFF CACHE BOOL "Enable support for Just-in-time compiling.")

if(${CMAKE_SYSTEM_NAME} MATCHES Linux|NetBSD)
    set(PCRE2_SUPPORT_JIT_SEALLOC OFF CACHE BOOL "Enable SELinux compatible execmem allocator in JIT (experimental).")
else()
    set(PCRE2_SUPPORT_JIT_SEALLOC IGNORE)
endif()

set(PCRE2GREP_SUPPORT_JIT ON CACHE BOOL "Enable use of Just-in-time compiling in pcre2grep.")
set(PCRE2GREP_SUPPORT_CALLOUT ON CACHE BOOL "Enable callout string support in pcre2grep.")
set(PCRE2GREP_SUPPORT_CALLOUT_FORK ON CACHE BOOL "Enable callout string fork support in pcre2grep.")
set(PCRE2_SUPPORT_UNICODE ON CACHE BOOL "Enable support for Unicode and UTF-8/UTF-16/UTF-32 encoding.")
set(PCRE2_SUPPORT_BSR_ANYCRLF OFF CACHE BOOL "ON=Backslash-R matches only LF CR and CRLF, OFF=Backslash-R matches all Unicode Linebreaks")
set(PCRE2_NEVER_BACKSLASH_C OFF CACHE BOOL "If ON, backslash-C (upper case C) is locked out.")
set(PCRE2_SUPPORT_VALGRIND OFF CACHE BOOL "Enable Valgrind support.")

if(MINGW)
    option(NON_STANDARD_LIB_PREFIX "ON=Shared libraries built in mingw will be named pcre2.dll, etc., instead of libpcre2.dll, etc." OFF)
    option(NON_STANDARD_LIB_SUFFIX "ON=Shared libraries built in mingw will be named libpcre2-0.dll, etc., instead of libpcre2.dll, etc." OFF)
endif()

# 

set(NEWLINE_DEFAULT "")

if(PCRE2_NEWLINE STREQUAL "CR")
    set(NEWLINE_DEFAULT "1")
elseif(PCRE2_NEWLINE STREQUAL "LF")
    set(NEWLINE_DEFAULT "2")
elseif(PCRE2_NEWLINE STREQUAL "CRLF")
    set(NEWLINE_DEFAULT "3")
elseif(PCRE2_NEWLINE STREQUAL "ANY")
    set(NEWLINE_DEFAULT "4")
elseif(PCRE2_NEWLINE STREQUAL "ANYCRLF")
    set(NEWLINE_DEFAULT "5")
elseif(PCRE2_NEWLINE STREQUAL "NUL")
    set(NEWLINE_DEFAULT "6")
else()
    message(FATAL_ERROR "The PCRE2_NEWLINE variable must be set to one of the following values: \"LF\", \"CR\", \"CRLF\", \"ANY\", \"ANYCRLF\".")
endif()

# Some tests

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckIncludeFile)

check_include_file(assert.h HAVE_ASSERT_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(windows.h HAVE_WINDOWS_H)

check_symbol_exists(bcopy "strings.h" HAVE_BCOPY)
check_symbol_exists(memfd_create "sys/mman.h" HAVE_MEMFD_CREATE)
check_symbol_exists(memmove "string.h" HAVE_MEMMOVE)
check_symbol_exists(secure_getenv "stdlib.h" HAVE_SECURE_GETENV)
check_symbol_exists(strerror "string.h" HAVE_STRERROR)

check_c_source_compiles(
  "int main(void) { char buf[128] __attribute__((uninitialized)); (void)buf; return 0; }"
  HAVE_ATTRIBUTE_UNINITIALIZED
)

check_c_source_compiles(
  [=[
  extern __attribute__ ((visibility ("default"))) int f(void);
  int main(void) { return f(); }
  int f(void) { return 42; }
  ]=]
  HAVE_VISIBILITY
)

if(HAVE_VISIBILITY)
  set(PCRE2_EXPORT [=[__attribute__ ((visibility ("default")))]=])
else()
  set(PCRE2_EXPORT)
endif()

check_c_source_compiles("int main(void) { __assume(1); return 0; }" HAVE_BUILTIN_ASSUME)

check_c_source_compiles(
  [=[
  #include <stddef.h>
  int main(void) { int a,b; size_t m; __builtin_mul_overflow(a,b,&m); return 0; }
  ]=]
  HAVE_BUILTIN_MUL_OVERFLOW
)

check_c_source_compiles(
  "int main(int c, char *v[]) { if (c) __builtin_unreachable(); return (int)(*v[0]); }"
  HAVE_BUILTIN_UNREACHABLE
)

# # Check whether Intel CET is enabled, and if so, adjust compiler flags. This
# # code was written by PH, trying to imitate the logic from the autotools
# # configuration.

# check_c_source_compiles(
#   [=[
#   #ifndef __CET__
#   #error CET is not enabled
#   #endif
#   int main() { return 0; }
#   ]=]
#   INTEL_CET_ENABLED
# )

# if(INTEL_CET_ENABLED)
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mshstk")
# endif()

# Set up some dependencies first

configure_file(
    ${PCRE2_SOURCE_DIR}/src/pcre2_chartables.c.dist
    ${CMAKE_CURRENT_BINARY_DIR}/pcre2_chartables.c
    COPYONLY
)

configure_file(
    ${PCRE2_SOURCE_DIR}/config-cmake.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/interface/config.h
    @ONLY
)

configure_file(
    ${PCRE2_SOURCE_DIR}/src/pcre2.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/interface/pcre2.h
    @ONLY
)

# Define our library

list(APPEND PCRE2_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/interface/pcre2.h)

list(APPEND PCRE2_SOURCES
    ${PCRE2_SOURCE_DIR}/src/pcre2_auto_possess.c
    ${CMAKE_CURRENT_BINARY_DIR}/pcre2_chartables.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_chkdint.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_compile.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_compile_class.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_config.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_context.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_convert.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_dfa_match.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_error.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_extuni.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_find_bracket.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_jit_compile.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_maketables.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_match.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_match_data.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_newline.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_ord2utf.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_pattern_info.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_script_run.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_serialize.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_string_utils.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_study.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_substitute.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_substring.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_tables.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_ucd.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_valid_utf.c
    ${PCRE2_SOURCE_DIR}/src/pcre2_xclass.c
)

add_library(pcre2s OBJECT)

target_sources(pcre2s
    PRIVATE ${PCRE2_SOURCES}
    PUBLIC
    FILE_SET pcre2_headers TYPE HEADERS
    BASE_DIRS ${PCRE2_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/interface
    FILES ${PCRE2_HEADERS}
)

target_compile_definitions(pcre2s PUBLIC PCRE2_CODE_UNIT_WIDTH=8 HAVE_CONFIG_H)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(pcre2s PUBLIC PCRE2_STATIC)
endif()

target_include_directories(pcre2s PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/interface ${PCRE2_SOURCE_DIR}/src)

if(PCRE2_STATIC_PIC)
    set_target_properties(pcre2s PROPERTIES POSITION_INDEPENDENT_CODE 1)
endif()

# # Installation and config files

# include(CMakePackageConfigHelpers)
# include(GenerateExportHeader)

# # Install rules
# install(TARGETS pcre2s
#     EXPORT pcre2s
#     FILE_SET pcre2_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# if(MSVC AND BUILD_SHARED_LIBS)
#     install(
#         FILES $<TARGET_PDB_FILE:pcre2s>
#         DESTINATION ${CMAKE_INSTALL_LIBDIR}
#         OPTIONAL)
# endif()

# install(EXPORT pcre2s
#     NAMESPACE pcre2s::
#     FILE "pcre2s-targets.cmake"
#     DESTINATION lib/cmake/pcre2s)

# configure_package_config_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/pcre2s-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/pcre2s/pcre2s-config.cmake
#     INSTALL_DESTINATION lib/cmake/pcre2s)

# install(
#     FILES "${CMAKE_CURRENT_BINARY_DIR}/pcre2s/pcre2s-config.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/pcre2s/pcre2s-config-version.cmake"
#     DESTINATION lib/cmake/pcre2s)

# set_target_properties(
#     pcre2s
#     PROPERTIES VERSION ${PCRE2_VERSION}
#     SOVERSION ${PCRE2_VERSION}
#     INTERFACE_pcre2s_MAJOR_VERSION ${PCRE2_MAJOR})

# set_property(
#     TARGET pcre2s
#     APPEND
#     PROPERTY COMPATIBLE_INTERFACE_STRING pcre2s_MAJOR_VERSION)

# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/pcre2s/pcre2s-config-version.cmake"
#     VERSION "${PCRE2_VERSION}"
#     COMPATIBILITY AnyNewerVersion)

# # Testing

# if(PROJECT_IS_TOP_LEVEL)
#     include(CTest)

#     if(BUILD_TESTING)
#         add_subdirectory(test)
#     endif()
# endif()

# The problem is, find_package(PCRE2) does not work
# and using pkg-config results in linking to a shared library
# causing all kinds of trouble later on

find_path(PCRE2_INCLUDEDIR NAMES pcre2.h HINTS "C:/Program Files (x86)/PCRE2/include" REQUIRED)
find_library(PCRE2_LIBRARY NAMES pcre2-8-static libpcre2-8.a HINTS "C:/Program Files (x86)/PCRE2/lib" REQUIRED)

add_library(pcre2-8 IMPORTED STATIC)
target_include_directories(pcre2-8 INTERFACE ${PCRE2_INCLUDEDIR})
target_compile_definitions(pcre2-8 INTERFACE PCRE2_STATIC)
set_target_properties(pcre2-8 PROPERTIES IMPORTED_LOCATION ${PCRE2_LIBRARY})
set_target_properties(pcre2-8 PROPERTIES IMPORTED_IMPLIB ${PCRE2_LIBRARY})

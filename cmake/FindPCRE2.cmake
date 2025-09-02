set(PCRE2_USE_STATIC_LIBS ON)

# The cmake config files for pcre2 are broken

include(FindPkgConfig)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(PCRE2 IMPORTED_TARGET libpcre2-8)

	if(PCRE2_FOUND)
		message(STATUS "Using pcre2 found using pkg-config")

		add_library(pcre2-8 ALIAS PkgConfig::PCRE2)
	endif()
endif()

if(NOT PCRE2_FOUND)
	find_path(PCRE2_INCLUDEDIR NAMES pcre2.h HINTS "C:/Program Files (x86)/PCRE2/include" REQUIRED)
	find_library(PCRE2_LIBRARY NAMES pcre2-8-static HINTS "C:/Program Files (x86)/PCRE2/lib" REQUIRED)

	add_library(pcre2-8 IMPORTED STATIC)
	target_include_directories(pcre2-8 INTERFACE ${PCRE2_INCLUDEDIR})
	target_compile_definitions(pcre2-8 INTERFACE PCRE2_STATIC)
	# target_link_libraries(pcre2-8 INTERFACE ${PCRE2_LIBRARY})
	set_target_properties(pcre2-8 PROPERTIES IMPORTED_LOCATION ${PCRE2_LIBRARY})
	set_target_properties(pcre2-8 PROPERTIES IMPORTED_IMPLIB ${PCRE2_LIBRARY})
endif()

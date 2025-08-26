set(PCRE2_USE_STATIC_LIBS ON)

# The cmake config files for pcre2 are broken
# find_package(pcre2 CONFIG COMPONENTS 8BIT)

if(PCRE2_FOUND)
	message(STATUS "Using pcre2 found using find_package")
else()
	include(FindPkgConfig)

	if(PKG_CONFIG_FOUND)
		pkg_check_modules(PCRE2 IMPORTED_TARGET libpcre2-8)

		if(PCRE2_FOUND)
			message(STATUS "Using pcre2 found using pkg-config")

			add_library(pcre2-8 ALIAS PkgConfig::PCRE2)
		endif()
	endif()
endif()

if(NOT PCRE2_FOUND)
	find_path(PCRE2_INCLUDEDIR NAMES pcre2.h HINTS "C:/Program Files (x86)/PCRE2/include" REQUIRED)
	find_library(PCRE2_LIBRARY NAMES pcre2-8-static pcre2-8 HINTS "C:/Program Files (x86)/PCRE2/lib" REQUIRED)

	add_library(pcre2-8 SHARED IMPORTED)
	set_target_properties(pcre2-8 PROPERTIES INCLUDE_DIRECTORIES ${PCRE2_INCLUDEDIR})
	set_target_properties(pcre2-8 PROPERTIES IMPORTED_LOCATION ${PCRE2_LIBRARY})
	if(WIN32)
		set_target_properties(pcre2-8 PROPERTIES IMPORTED_IMPLIB ${PCRE2_LIBRARY})
	endif()
endif()
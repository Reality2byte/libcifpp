set(PCRE2_USE_STATIC_LIBS ON)

find_package(pcre2 CONFIG)

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
	message(STATUS "Using pcre2 using FetchContent")

	set(PCRE2_BUILD_TESTS OFF)
	FetchContent_Declare(
		pcre2
		GIT_REPOSITORY https://github.com/PCRE2Project/pcre2
		GIT_TAG pcre2-10.45
		EXCLUDE_FROM_ALL)
	FetchContent_MakeAvailable(pcre2)

	# add_subdirectory(${pcre2_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()
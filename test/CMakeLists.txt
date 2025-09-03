# We're using the older version 2 of Catch2

if(NOT(Catch2_FOUND OR TARGET Catch2))
	find_package(Catch2 3 QUIET)

	if(NOT Catch2_FOUND)
		include(FetchContent)

		FetchContent_Declare(
			Catch2
			GIT_REPOSITORY https://github.com/catchorg/Catch2.git
			GIT_TAG GIT_TAG v3.5.2)

		FetchContent_MakeAvailable(Catch2)
	endif()
endif()

list(
	APPEND
	CIFPP_tests
	unit-v2
	unit-3d
	model
	query
	rename-compound
	sugar
	spinner
	# reconstruction
	validate-pdbx
	)

add_library(test-main OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/test-main.cpp")

target_link_libraries(test-main cifpp::cifpp Catch2::Catch2)

foreach(CIFPP_TEST IN LISTS CIFPP_tests)
	set(CIFPP_TEST "${CIFPP_TEST}-test")
	set(CIFPP_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${CIFPP_TEST}.cpp")

	add_executable(
		${CIFPP_TEST} ${CIFPP_TEST_SOURCE} $<TARGET_OBJECTS:test-main>)

	target_link_libraries(${CIFPP_TEST} PRIVATE cifpp::cifpp Catch2::Catch2)
	target_include_directories(${CIFPP_TEST} PRIVATE "${EIGEN_INCLUDE_DIR}")

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(${CIFPP_TEST} PRIVATE /EHsc)
	endif()

	add_custom_target(
		"run-${CIFPP_TEST}"
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Run${CIFPP_TEST}.touch ${CIFPP_TEST})

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Run${CIFPP_TEST}.touch
		COMMAND $<TARGET_FILE:${CIFPP_TEST}> --data-dir
		${CMAKE_CURRENT_SOURCE_DIR})

	add_test(NAME ${CIFPP_TEST} COMMAND $<TARGET_FILE:${CIFPP_TEST}> --data-dir
		${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
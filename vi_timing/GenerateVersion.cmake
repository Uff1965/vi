cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

message(STATUS "***** Generating version information...")

find_package(Git QUIET)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --tags --long --always --abbrev --dirty --broken
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_DESCRIBE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_COMMIT
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
		COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=iso
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_DATETIME
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	string(FIND "${GIT_DESCRIBE}" "-dirty" DIRTY_POS)
	if(DIRTY_POS EQUAL -1)
		set(GIT_IS_DIRTY 0)
	else()
		set(GIT_IS_DIRTY 1)
	endif()

	string(REGEX MATCH "[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?(\\.[0-9]+)?" GIT_VERSION_NUMBER "${GIT_DESCRIBE}")

else()
	# Резервные значения
	set(GIT_DESCRIBE "")
	set(GIT_COMMIT "unknown")
	set(GIT_DATETIME "unknown")
	set(GIT_IS_DIRTY 0)
	set(GIT_VERSION_NUMBER "0.0.0")
endif()

function(ensure_file_has_content target_file expected_content)
    if(EXISTS "${target_file}")
        file(READ "${target_file}" existing_content)
        if(NOT existing_content STREQUAL expected_content)
            file(WRITE "${target_file}" "${expected_content}")
        endif()
    else()
        file(WRITE "${target_file}" "${expected_content}")
    endif()
endfunction()

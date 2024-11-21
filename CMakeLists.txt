cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

function(vi_macro_show M)
	if(ARGV1)
		set(msg "${ARGV1}: ")
	else()
		set(msg "")
	endif()

	if(${M})
		message("${msg}${M} = \'${${M}}\'")
	else()
		message("${msg}${M} = <not defined>")
	endif()
endfunction()

project(vi)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

#Requires CMake version no older than 3.21.	cmake_path(SET VI_ROOT_DIR ${PROJECT_SOURCE_DIR})
if(NOT DEFINED VI_OUT_DIR)
	get_filename_component(VI_OUT_DIR "${PROJECT_SOURCE_DIR}/lib/." DIRECTORY)
endif()

add_subdirectory(vi_timing)

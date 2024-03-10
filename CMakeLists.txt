cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(vi)

function(vi_show m)
	if(DEFINED ${m})
		message("${m}: \'${${m}}\'")
	else()
		message("${m}: <undefined>")
	endif()
endfunction()

#Requires CMake version no older than 3.21.	need cmake_path(SET VI_ROOT_DIR ${PROJECT_SOURCE_DIR})
get_filename_component(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)

vi_show(VI_ROOT_DIR)

add_subdirectory(vi_timing)

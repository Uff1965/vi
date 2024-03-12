cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(vi)

function(vi_macro_show M)
	if(DEFINED ${M})
		message("${M}: \'${${M}}\'")
	else()
		message("${M}: <not defined>")
	endif()
endfunction()

vi_macro_show(CMAKE_BUILD_TYPE)
vi_macro_show(PROJECT_NAME)
vi_macro_show(CMAKE_PROJECT_NAME)

#Requires CMake version no older than 3.21.	cmake_path(SET VI_ROOT_DIR ${PROJECT_SOURCE_DIR})
get_filename_component(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)

add_subdirectory(vi_timing)

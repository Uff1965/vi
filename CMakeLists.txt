cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(vi)

#Requires CMake version no older than 3.21.	need cmake_path(SET VI_ROOT_DIR ${PROJECT_SOURCE_DIR})
get_filename_component(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)

add_subdirectory(vi_timing)

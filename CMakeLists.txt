cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(vi)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

get_filename_component(VI_OUT_DIR "${PROJECT_SOURCE_DIR}/lib/." DIRECTORY)
get_filename_component(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}/." DIRECTORY)

add_subdirectory(vi_timing)

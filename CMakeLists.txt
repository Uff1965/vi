cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(vi)

set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Enable folder structure in IDEs

set(CMAKE_CONFIGURATION_TYPES Debug Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

get_filename_component(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}/." DIRECTORY)
set(VI_OUT_DIR "${VI_ROOT_DIR}/lib")

add_subdirectory(vi_timing)

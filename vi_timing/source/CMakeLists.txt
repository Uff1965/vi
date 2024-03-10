cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(vi_timing VERSION 3.1.1)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

vi_show(VI_ROOT_DIR)

if(NOT DEFINED VI_ROOT_DIR)
#Requires CMake version no older than 3.21	cmake_path(GET PROJECT_SOURCE_DIR PARENT_PATH VI_ROOT_DIR)
	get_filename_component(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}/..")
endif()

#[[*** Requires CMake version no older than 3.21 ***
cmake_path(SET VI_INTERFACE_DIR ${VI_ROOT_DIR})
cmake_path(SET VI_SOURCE_DIR ${PROJECT_SOURCE_DIR})
cmake_path(APPEND VI_ROOT_DIR "lib" OUTPUT_VARIABLE VI_OUT_DIR)
]]
get_filename_component(VI_INTERFACE_DIR "${VI_ROOT_DIR}" ABSOLUTE)
get_filename_component(VI_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
get_filename_component(VI_OUT_DIR "${VI_ROOT_DIR}/lib" ABSOLUTE)

### Files #####################################################################
set(FILE_GROUP
  "${VI_INTERFACE_DIR}/vi/common.h"
  "${VI_INTERFACE_DIR}/vi/timing.h"
  "${VI_INTERFACE_DIR}/vi/vi_timing_proxy.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "${VI_SOURCE_DIR}/timing.cpp"
  "${VI_SOURCE_DIR}/report.cpp"
)
source_group("Source files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

vi_show(VI_OUT_DIR)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
  OUTPUT_NAME_DEBUG   "${PROJECT_NAME}_d"
  LIBRARY_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"

  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
)

### Compiler ##################################################################
target_include_directories(${PROJECT_NAME}
PUBLIC
  ${VI_INTERFACE_DIR}
)

target_compile_definitions(${PROJECT_NAME}
PRIVATE
  WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
  NOMINMAX # WinAPI
  $<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
  $<$<CONFIG:Release>: NDEBUG>
)

if (WIN32)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    /MP /W3 /nologo /EHsc /Zi 
    $<$<CONFIG:Release>: /MT  /O2 /Oi /GL /Gy>
    $<$<CONFIG:Debug>:   /MTd /Od /RTC1>
  )
elseif (UNIX)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden
    $<$<COMPILE_LANGUAGE:CXX>:-fPIC>
    $<$<CONFIG:Release>:-O3 -s>
  )
endif()

### Linker ####################################################################
if (WIN32)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    /SUBSYSTEM:WINDOWS /DEBUG
    $<$<CONFIG:Release>: /INCREMENTAL:NO /LTCG /OPT:REF /OPT:ICF>
    $<$<CONFIG:Debug>:   /INCREMENTAL>
  )
elseif (UNIX)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    -Wl,--exclude-libs,ALL
    -Wno-psabi
  )

  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rt
  PUBLIC
    atomic
  )
endif()

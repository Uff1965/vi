cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(vi_timing_test)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

cmake_path(GET PROJECT_SOURCE_DIR PARENT_PATH VI_TIMING_ROOT_DIR)
cmake_path(SET VI_SOURCE_DIR ${PROJECT_SOURCE_DIR})
cmake_path(SET VI_INTERFACE_DIR ${VI_ROOT_DIR})
cmake_path(APPEND VI_ROOT_DIR "lib" OUTPUT_VARIABLE VI_OUT_DIR)

### Files #####################################################################
set(FILE_GROUP
  "${VI_SOURCE_DIR}/test.cpp"
  "${VI_SOURCE_DIR}/header.h"
  "${VI_SOURCE_DIR}/source.c"
)
source_group("Source Files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "${VI_INTERFACE_DIR}/vi/timing.h"
  "${VI_INTERFACE_DIR}/vi/vi_timing_proxy.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
  OUTPUT_NAME_DEBUG   "${PROJECT_NAME}_d"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
)

### Compiler ##################################################################
target_compile_definitions(${PROJECT_NAME}
PRIVATE
  $<$<CONFIG:Debug>:_DEBUG> # Microsoft debug RTL
  $<$<CONFIG:Release>:NDEBUG>
)

if (WIN32)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    /MP /W3 /nologo /EHsc /Zi
    /experimental:c11atomics # "Atomics are available in Visual Studio 2022 version 17.5 Preview 2 with the /experimental:c11atomics flag, in /std:c11 mode or later."
    $<$<CONFIG:Release>: /MT  /O2 /Oi /GL /Gy>
    $<$<CONFIG:Debug>:   /MTd /Od /RTC1>
  )
elseif (UNIX)

  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden
    $<$<COMPILE_LANGUAGE:CXX>:-fPIC>
    $<$<CONFIG:Release>:-O3 -ggdb3 -s>
  )
endif()

### Linker ####################################################################

target_link_libraries(${PROJECT_NAME}
PRIVATE
	vi_timing
)

if (WIN32)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    /SUBSYSTEM:CONSOLE /DEBUG
    $<$<CONFIG:Release>: /INCREMENTAL:NO /LTCG /OPT:REF /OPT:ICF>
    $<$<CONFIG:Debug>:   /INCREMENTAL>
  )
elseif (UNIX)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    -Wl,--exclude-libs,ALL
  )

  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rt
    pthread
  PUBLIC
    atomic
  )
endif()

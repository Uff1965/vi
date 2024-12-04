cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(vi_timing VERSION 0.14.1)

if(NOT DEFINED VI_OUT_DIR)
	get_filename_component(VI_OUT_DIR "${PROJECT_SOURCE_DIR}/lib/." DIRECTORY)
endif()

get_filename_component(VI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/source/." DIRECTORY)

if(NOT DEFINED VI_TM_BUILD_SHARED)
    if(BUILD_SHARED_LIBS)
        set(VI_TM_BUILD_SHARED ON)
    endif()
endif()

### Files #####################################################################
set(SOURCE_FILES)

set(FILE_GROUP
  "${PROJECT_SOURCE_DIR}/common.h"
  "${PROJECT_SOURCE_DIR}/vi_timing.h"
  "${PROJECT_SOURCE_DIR}/vi_timing_proxy.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "${VI_SOURCE_DIR}/timing.cpp"
  "${VI_SOURCE_DIR}/timing_aux.cpp"
)
source_group("Source files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(OUTPUT_NAME_SUFFIX "")

if(VI_TM_BUILD_SHARED)
    set(BUILD_TYPE "SHARED")
    string(APPEND OUTPUT_NAME_SUFFIX "s")
else()
    set(BUILD_TYPE "STATIC")
endif()

add_library(${PROJECT_NAME} ${BUILD_TYPE} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES DEFINE_SYMBOL "VI_TM_EXPORTS")

#add_custom_command(
#    TARGET ${PROJECT_NAME}
#    PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -E touch_nocreate "${VI_SOURCE_DIR}/timing_aux.cpp"
#    COMMENT "pre-build command: Change the modification time of the timing_aux.cpp file so that it recompiles and updates BUILD_NUMBER."
#)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF

  OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<$<BOOL:${OUTPUT_NAME_SUFFIX}>:_>${OUTPUT_NAME_SUFFIX}"
  OUTPUT_NAME_DEBUG "${PROJECT_NAME}_${OUTPUT_NAME_SUFFIX}d"

  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"

# Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config) append a per-configuration subdirectory to the specified directory unless a generator expression is used.
#  LIBRARY_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
#  ARCHIVE_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
#  RUNTIME_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
)

### Compiler ##################################################################
target_include_directories(${PROJECT_NAME}
PUBLIC
  ${PROJECT_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME}
PRIVATE
  $<$<CONFIG:Release>: NDEBUG>
)

if(VI_TM_BUILD_SHARED)
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        "VI_TM_SHARED"
    )
endif()

if (WIN32)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
      NOMINMAX # WinAPI
      $<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
    )

  target_compile_options(${PROJECT_NAME}
  PRIVATE
    /MP /W3 /nologo /EHsc /Zi 
    $<$<CONFIG:Release>: /MD  /O2 /Oi /GL /Gy>
    $<$<CONFIG:Debug>:   /MDd /Od /RTC1>
  )
elseif (UNIX)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden
    -Wno-psabi #suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    -Wl,--exclude-libs,ALL
    $<$<COMPILE_LANGUAGE:CXX>: -fPIC>
    $<$<CONFIG:Release>: -O3 -s>
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
  )

  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rt
  PUBLIC
    atomic
  )
endif()

add_subdirectory(test)

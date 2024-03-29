cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(vi_timing VERSION 3.1.1)

if(NOT DEFINED VI_OUT_DIR)
	get_filename_component(VI_OUT_DIR "${PROJECT_SOURCE_DIR}/lib/." DIRECTORY)
endif()

get_filename_component(VI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/source/." DIRECTORY)

### Files #####################################################################
set(SOURCE_FILES)

set(FILE_GROUP
  "${PROJECT_SOURCE_DIR}/common.h"
  "${PROJECT_SOURCE_DIR}/timing.h"
  "${PROJECT_SOURCE_DIR}/vi_timing_proxy.h"
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

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
  OUTPUT_NAME_DEBUG   "${PROJECT_NAME}_d"

  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"

  LIBRARY_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY	"${VI_OUT_DIR}"
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
    $<$<CONFIG:Release>: /MT  /O2 /Oi /GL /Gy>
    $<$<CONFIG:Debug>:   /MTd /Od /RTC1>
  )
elseif (UNIX)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden
    -Wno-psabi #suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
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

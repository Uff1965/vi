cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(vi_timing_test)

### Files #####################################################################
set(SOURCE_FILES)

set(FILE_GROUP
  "${PROJECT_SOURCE_DIR}/test.cpp"
  "${PROJECT_SOURCE_DIR}/header.h"
  "${PROJECT_SOURCE_DIR}/source.c"
)
source_group("Source Files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "../timing.h"
  "../vi_timing_proxy.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

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

add_dependencies(${PROJECT_NAME}
	vi_timing
)

## Compiler ##################################################################
target_compile_definitions(${PROJECT_NAME}
PRIVATE
  $<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
  $<$<CONFIG:Release>: NDEBUG>
)

if (WIN32)
   target_compile_definitions(${PROJECT_NAME}
   PRIVATE
     WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
     NOMINMAX # WinAPI
   )

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
    -Wno-psabi #suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    $<$<COMPILE_LANGUAGE:CXX>: -fPIC>
    $<$<CONFIG:Release>: -O3 -ggdb3 -s>
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

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(vi_timing_test)

### Files #####################################################################
set(SOURCE_FILES)

if(MSVC)
    set(FILE_GROUP
      "vi_timing_test.pvsconfig"
    )
    source_group("Extras" FILES ${FILE_GROUP})
    list(APPEND SOURCE_FILES ${FILE_GROUP})
endif()

set(FILE_GROUP
  "header.h"
  "main.cpp"
  "source.c"
)
source_group("Source Files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "../vi_timing.h"
  "../vi_timing_c.h"
  "../vi_timing_proxy.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(OUTPUT_NAME_SUFFIX "")
if(BUILD_SHARED_LIBS)
    string(APPEND OUTPUT_NAME_SUFFIX "s")
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<$<BOOL:${OUTPUT_NAME_SUFFIX}>:_>${OUTPUT_NAME_SUFFIX}"
  OUTPUT_NAME_DEBUG   "${PROJECT_NAME}_${OUTPUT_NAME_SUFFIX}d"

  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
)

## Compiler ##################################################################
target_compile_definitions(${PROJECT_NAME}
PRIVATE
  $<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
  $<$<CONFIG:Release>: NDEBUG> # Disable debug assertions in release builds.
)

if (WIN32)
   target_compile_definitions(${PROJECT_NAME}
   PRIVATE
     WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
     NOMINMAX # WinAPI
   )

  target_compile_options(${PROJECT_NAME}
  PRIVATE
    /MP /W3 /nologo /EHsc # Enable multi-processor compilation, set warning level 3, disable logo, enable C++ exceptions.
    /Zi # Generate complete debugging information.
    /Zc:__cplusplus # By default, Visual Studio always returns the value 199711L for the __cplusplus preprocessor macro.
    /experimental:c11atomics # "Atomics are available in Visual Studio 2022 version 17.5 Preview 2 with the /experimental:c11atomics flag, in /std:c11 mode or later."
    $<$<CONFIG:Release>: /MD  /O2 /Oi /GL /Gy> # Optimize for speed, use multi-threaded DLL runtime, inline functions, enable link-time code generation, enable function-level linking.
    $<$<CONFIG:Debug>:   /MDd /Od /RTC1> # Use multi-threaded DLL debug runtime, enable runtime checks, no optimizations.
  )
elseif (UNIX)
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden # Hide symbols by default.
    -Wno-psabi #suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    $<$<COMPILE_LANGUAGE:CXX>: -fPIC> # Position independent code for C++.
    $<$<CONFIG:Release>: -O3 -ggdb3 -s> # Optimize for speed, generate debug info, strip symbols.
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
    $<$<CONFIG:Release>: /INCREMENTAL:NO /LTCG /OPT:REF /OPT:ICF> # Disable incremental linking, enable link-time code generation, optimize for speed and size.
    $<$<CONFIG:Debug>:   /INCREMENTAL> # Enable incremental linking for debug builds.
  )
elseif (UNIX)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    -Wl,--exclude-libs,ALL # Exclude all symbols from static libraries.
  )

  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rt # Real-time extensions library for UNIX-like systems.
    pthread # POSIX threads library for UNIX-like systems.
    atomic # C11 atomic operations library for UNIX-like systems.
  )
endif()

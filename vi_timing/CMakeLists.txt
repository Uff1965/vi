cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/.git/HEAD"
    "${CMAKE_SOURCE_DIR}/.git_tag"
#directory!    "${CMAKE_SOURCE_DIR}/.git/refs/tags"
    "${CMAKE_SOURCE_DIR}/.git/packed-refs"
)

include("${CMAKE_CURRENT_LIST_DIR}/GenerateVersion.cmake")


project(vi_timing VERSION ${GIT_VERSION_NUMBER})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/version.h" @ONLY)

option(VI_TM_BUILD_SHARED "Build vi_timing as shared library" OFF)
set(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}/../" CACHE PATH "Path to root directory (usual 'vi/').")
set(VI_OUT_DIR "${VI_ROOT_DIR}/lib/" CACHE PATH "Output directory for libraries")

### Files #####################################################################
set(SOURCE_FILES)

if(MSVC)
    set(FILE_GROUP
      "${VI_ROOT_DIR}/vi_timing/vi_timing.pvsconfig"
      "${VI_ROOT_DIR}/vi_timing/vi_timing.suppress"
    )
    source_group("Extras" FILES ${FILE_GROUP})
    list(APPEND SOURCE_FILES ${FILE_GROUP})
endif()

set(FILE_GROUP
  "${VI_ROOT_DIR}/vi_timing/vi_timing.h"
  "${VI_ROOT_DIR}/vi_timing/vi_timing_c.h"
  "${VI_ROOT_DIR}/vi_timing/vi_timing_proxy.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "source/build_number_maker.h"
  "source/misc.h"
)
source_group("Header files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "source/build_number_maker.cpp"
  "source/clock.cpp"
  "source/misc.cpp"
  "source/props.cpp"
  "source/report.cpp"
  "source/timing.cpp"
)
source_group("Source files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(OUTPUT_NAME_SUFFIX "")

if(VI_TM_BUILD_SHARED)
    string(APPEND OUTPUT_NAME_SUFFIX "s")
    set(TYPE_LIBRARY "SHARED")
endif()

add_library(${PROJECT_NAME} ${TYPE_LIBRARY} ${SOURCE_FILES})


set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF

  DEFINE_SYMBOL "VI_TM_EXPORTS"

  OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<$<BOOL:${OUTPUT_NAME_SUFFIX}>:_>${OUTPUT_NAME_SUFFIX}"
  OUTPUT_NAME_DEBUG "${PROJECT_NAME}_${OUTPUT_NAME_SUFFIX}d"
#  OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<$<BOOL:${OUTPUT_NAME_SUFFIX}>:_>${OUTPUT_NAME_SUFFIX}.${VI_TM_GIT_TAG}"
#  OUTPUT_NAME_DEBUG "${PROJECT_NAME}_${OUTPUT_NAME_SUFFIX}d.${VI_TM_GIT_TAG}"

  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"

  POSITION_INDEPENDENT_CODE ON # Position Independent Code (-fPIC) for shared libraries on UNIX.
)

### Compiler ##################################################################
target_include_directories(${PROJECT_NAME}
PUBLIC
  ${VI_ROOT_DIR}
)

target_compile_definitions(${PROJECT_NAME}
PRIVATE
  $<$<CONFIG:Release>: NDEBUG>
  $<$<CONFIG:Debug>: VI_TM_DEBUG=1>
)

if(VI_TM_BUILD_SHARED)
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        VI_TM_SHARED=1
    )
endif()

#target_precompile_headers(${PROJECT_NAME}
#PRIVATE
#    "source/pch.hpp"
#)

if (WIN32)
    set_source_files_properties("source/timing.cpp" PROPERTIES
        COMPILE_FLAGS "/fp:fast" # Enable fast floating-point model for 'timing.cpp'
        SKIP_PRECOMPILE_HEADERS ON
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
      NOMINMAX # WinAPI
      WIN32 # Define WIN32 for Windows builds.
      _WINDOWS # Define _WINDOWS for Windows builds.
      $<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
    )

    target_compile_options(${PROJECT_NAME}
    PRIVATE
        /MP /nologo # Enable multi-processor compilation, disable logo.
        /W4 # Set warning level 4.
        /EHsc # Enable C++ exceptions.
        /arch:AVX2 # Enable use AVX2 instruction set.
        /Zi # Generate debug info
        /Zc:__cplusplus # By default, Visual Studio always returns the value 199711L for the __cplusplus preprocessor macro.
        $<$<CONFIG:Release>: /Ox /GL> # Ox include /O2 /Ob2 /Og /Oi /Ot /Oy /Gs /GF /Gy
        $<$<CONFIG:Debug>: /Od /RTC1 /Zi /Ob0> # Enable runtime checks, debug info, no optimizations, disable function inlining.
    )

    if(VI_TM_BUILD_SHARED)
        target_compile_options(${PROJECT_NAME}
        PRIVATE
        $<$<CONFIG:Release>: /MT>
        $<$<CONFIG:Debug>:   /MTd>
        )
    else()
        target_compile_options(${PROJECT_NAME}
        PRIVATE
        $<$<CONFIG:Release>: /MD>
        $<$<CONFIG:Debug>:   /MDd>
        )
    endif()
elseif (UNIX)
  set_source_files_properties("source/timing.cpp" PROPERTIES
    COMPILE_FLAGS "-ffast-math" # Allow floating-point optimizations that may violate strict IEEE or ISO rules for 'timing.cpp'.
    SKIP_PRECOMPILE_HEADERS ON
  )

  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden # Hide symbols by default, only export those marked with VI_TM_EXPORTS.
    -Wno-psabi # suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    -march=native # Switch on all instruction sets supported by the host CPU. Support FMA, AVX, AVX2, etc.
    $<$<CONFIG:Release>: -O3 -s> # Optimize for speed, strip symbols.
  )
endif()

### Linker ####################################################################
if (WIN32)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    /SUBSYSTEM:WINDOWS # Set subsystem to Windows
    /DEBUG # Generate debug info.
    $<$<CONFIG:Release>: /INCREMENTAL:NO /LTCG /OPT:REF /OPT:ICF> # Disable incremental linking, link-time code generation, and optimize for size.
    $<$<CONFIG:Debug>:   /INCREMENTAL> # Enable incremental linking for Debug builds.
  )
elseif (UNIX)
  target_link_options(${PROJECT_NAME}
  PRIVATE
    -Wl,--exclude-libs,ALL # Exclude all static libraries from the shared library.
  )

  target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rt # Link against the real-time library for clock_gettime, etc.
    atomic # Link against the atomic library for atomic operations.
  )
endif()

add_subdirectory(test)

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(vi_timing)

if(NOT DEFINED VI_ROOT_DIR)
	set(VI_ROOT_DIR ${PROJECT_SOURCE_DIR}/..)
endif()

if(NOT DEFINED VI_OUT_DIR)
	get_filename_component(VI_OUT_DIR "${VI_ROOT_DIR}/lib/." DIRECTORY)
endif()

if((NOT DEFINED VI_TM_BUILD_SHARED) AND (BUILD_SHARED_LIBS))
    set(VI_TM_BUILD_SHARED ON)
endif()

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

  LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"

  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
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
        VI_TM_SHARED
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
    /MP /W4 /nologo /EHsc /Zi /arch:AVX2 /fp:fast # Enable multi-processor compilation, set warning level 4, disable logo, enable C++ exceptions, generate debug info, use AVX2 instruction set, and fast floating-point model.
    $<$<CONFIG:Release>: /O2 /Oi /GL /Gy> # Optimize for speed, inline functions, enable link-time code generation, enable function-level linking.
    $<$<CONFIG:Debug>:   /Od /RTC1> # Enable runtime checks, debug info, no optimizations.
    /Zc:__cplusplus # By default, Visual Studio always returns the value 199711L for the __cplusplus preprocessor macro.
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
  target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fvisibility=hidden #Hide symbols by default, only export those marked with VI_TM_EXPORTS.
    -Wno-psabi #suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    -march=native #Switch on all instruction sets supported by the host CPU. Support FMA, AVX, AVX2, etc.
    -ffast-math #Allow floating-point optimizations that may violate strict IEEE or ISO rules.
    $<$<COMPILE_LANGUAGE:CXX>: -fPIC> #Position Independent Code for shared libraries.
    $<$<CONFIG:Release>: -O3 -s> #Optimize for speed, strip symbols.
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

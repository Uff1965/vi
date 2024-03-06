#ifndef VI_TIMING_VI_COMMON_H
#	define VI_TIMING_VI_COMMON_H
#	pragma once

#ifdef __cplusplus
#	define VI_STD(s) std::s
#	define VI_MEMORY_ORDER(s) std::memory_order::s
#	define VI_NOEXCEPT noexcept
#	define VI_R_CAST(T, s) reinterpret_cast<T>(s)
#	define VI_S_CAST(T, s) static_cast<T>(s)
#else
#	define VI_STD(s) s
#	define VI_MEMORY_ORDER(s) s
#	define VI_NOEXCEPT
#	define VI_R_CAST(T, s) (T)s
#	define VI_S_CAST(T, s) (T)s
#endif

#ifdef _MSC_VER
#	define VI_FUNCNAME __FUNCTION__
#elif defined(__GNUC__)
#	define VI_FUNCNAME __PRETTY_FUNCTION__
#else
#	define VI_FUNCNAME __func__
#endif

#if defined(_MSC_VER)
#	define VI_OPTIMIZE_OFF _Pragma("optimize(\"\", off)")
#	define VI_OPTIMIZE_ON  _Pragma("optimize(\"\", on)")
#elif defined(__GNUC__)
#	define VI_OPTIMIZE_OFF _Pragma("GCC push_options") _Pragma("GCC optimize(\"O0\")")
#	define VI_OPTIMIZE_ON  _Pragma("GCC pop_options")
#else
#	define VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_ON
#	error Unknown compyler!
#endif

#define VI_STR_AUX( a, b ) a##b
#define VI_STR( a, b ) VI_STR_AUX( a, b )
#define VI_MAKE_VAR_NAME( prefix ) VI_STR( prefix, __LINE__ )

#endif // #ifndef VI_TIMING_VI_COMMON_H

#ifndef __VI_TIMING__VI_COMMON_H__
#	define __VI_TIMING__VI_COMMON_H__
#	pragma once

#ifdef __cplusplus
#	define VI_NOEXCEPT noexcept
#	define VI_STD(s) std::s
#	define VI_MEMORY_ORDER(s) std::memory_order::s
#	define VI_R_CAST(T, s) reinterpret_cast<T>(s)
#	define VI_S_CAST(T, s) static_cast<T>(s)
#else
#	define VI_NOEXCEPT
#	define VI_STD(s) s
#	define VI_MEMORY_ORDER(s) s
#	define VI_R_CAST(T, s) (T)s
#	define VI_S_CAST(T, s) (T)s
#endif

// Auxiliary definitions
#ifdef _MSC_VER
#	define  VI_FUNCNAME __FUNCTION__
#elif defined(__GNUC__)
#	define  VI_FUNCNAME __PRETTY_FUNCTION__
#else
#	define  VI_FUNCNAME __func__
#endif

#define VI_STR__( a, b ) a##b
#define VI_STR( a, b ) VI_STR__( a, b )
#define VI_MAKE_VAR( type, prefix ) type VI_STR( VI_STR( VI_STR(vi__, prefix), __), __LINE__ )

#endif // #ifndef __VI_TIMING__VI_COMMON_H__

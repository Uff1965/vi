/********************************************************************\
'vi_timing' is a small library for measuring the time execution of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was created to experiment for educational purposes. 
Do not expect much from it. If you spot a bug or can suggest any 
improvement to the code, please email me at eMail:programmer.amateur@proton.me.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. 
If not, see <https://www.gnu.org/licenses/gpl-3.0.html#license-text>.
\********************************************************************/

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
#	define VI_STD(foo_s) foo_s
#	define VI_MEMORY_ORDER(foo_s) foo_s
#	define VI_NOEXCEPT
#	define VI_R_CAST(T, foo_s) (T)foo_s
#	define VI_S_CAST(T, foo_s) (T)foo_s
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
#define VI_MAKE_UNIC_ID( prefix ) VI_STR( prefix, __LINE__ )

#endif // #ifndef VI_TIMING_VI_COMMON_H

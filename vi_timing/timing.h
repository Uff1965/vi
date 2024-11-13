/********************************************************************\
'vi_timing' is a compact library designed for measuring the execution time of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was developed for experimental and educational purposes.
Please temper your expectations accordingly. If you encounter any bugs or have
suggestions for improvements, kindly contact me at programmer.amateur@proton.me.

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

#ifndef VI_TIMING_VI_TIMING_H
#	define VI_TIMING_VI_TIMING_H
#	pragma once

#	define VI_TM_VERSION_MAJOR 0
#	define VI_TM_VERSION_MINOR 6
#	define VI_TM_VERSION_PATCH 1
#	define VI_TM_VERSION (((VI_TM_VERSION_MAJOR) * 1000U + (VI_TM_VERSION_MINOR)) * 1000U + (VI_TM_VERSION_PATCH))
#	define VI_TM_VERSION_STR VI_STR(VI_TM_VERSION_MAJOR) "." VI_STR(VI_TM_VERSION_MINOR) "." VI_STR(VI_TM_VERSION_PATCH)

#if defined(_WIN32)
#	include <Windows.h>
#elif defined(__linux__)
#	include <time.h> // for clock_gettime
#endif

#ifdef _MSC_VER
#	include <intrin.h>
#	pragma intrinsic(__rdtscp, _mm_lfence)
#elif defined(__GNUC__)
#	include <x86intrin.h>
#endif

#ifdef __cplusplus
#	include <atomic>
#	include <cstdint>
#	include <cstdio>
#	include <string>
#else
#	ifdef __STDC_NO_ATOMICS__
//		At the moment Atomics are available in Visual Studio 2022 with the /experimental:c11atomics flag.
//		"we left out support for some C11 optional features such as atomics" [Microsoft
//		https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#		error "Atomic objects and the atomic operation library are not supported."
#	endif
#	include <stdatomic.h>
#	include <stdint.h>
#	include <stdio.h>
#endif

#include "common.h"

// Define VI_TM_CALL, VI_TM_API and VI_SYS_CALL vvvvvvvvvvvvvv
#if defined(_WIN32) // Windows x86 or x64
#	define VI_SYS_CALL __cdecl
#	ifdef _WIN64
#		define VI_TM_CALL
#	else
#		define VI_TM_CALL __fastcall
#	endif
#	ifdef VI_TM_EXPORT
#		define VI_TM_API __declspec(dllexport)
#	elif defined(VI_TM_SHARED)
#		define VI_TM_API __declspec(dllimport)
#	else
#		define VI_TM_API
#	endif
#elif defined(__ANDROID__)
#	define VI_TM_DISABLE "Android not supported yet."
#elif defined (__linux__)
#	define VI_SYS_CALL
#	define VI_TM_CALL
#	ifdef VI_TM_EXPORT
#		define VI_TM_API __attribute__((visibility("default")))
#	else
#		define VI_TM_API
#	endif
#else
#	define VI_TM_DISABLE "Unknown platform!"
#endif
// Define VI_TM_CALL and VI_TM_API ^^^^^^^^^^^^^^^^^^^^^^^

typedef VI_STD(uint64_t) vi_tmTicks_t;
typedef int (VI_SYS_CALL *vi_tmLogRAW_t)(const char* name, vi_tmTicks_t time, VI_STD(size_t) amount, VI_STD(size_t) calls_cnt, void* data);
typedef int (VI_SYS_CALL *vi_tmLogSTR_t)(const char* str, void* data); // Must be compatible with std::fputs!
#ifdef __cplusplus
	using vi_tmAtomicTicks_t = std::atomic<vi_tmTicks_t>;
#else
	typedef _Atomic(vi_tmTicks_t) vi_tmAtomicTicks_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Definition of vi_tmGetTicks() function for different platforms. vvvvvvvvvvvv
#ifndef vi_tmGetTicks
#	if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__amd64__) // MSC or GCC on Intel
		static inline vi_tmTicks_t vi_tmGetTicks_impl(void)
		{	VI_STD(uint32_t) _; // áóäåò óäàë¸í îïòèìèçàòîðîì
			const VI_STD(uint64_t) result = __rdtscp(&_);
			//	«If software requires RDTSCP to be executed prior to execution of any subsequent instruction 
			//	(including any memory accesses), it can execute LFENCE immediately after RDTSCP» - 
			//	(Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes:
			//	1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4. Vol. 2B. P.4-553)
			_mm_lfence();
			return result;
		}
#	elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
		static inline vi_tmTicks_t vi_tmGetTicks_impl(void)
		{	VI_STD(uint64_t) result;
			__asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(result));
			return result;
		}
#	elif defined(_WIN32) // Windows
		static inline vi_tmTicks_t vi_tmGetTicks_impl(void)
		{	LARGE_INTEGER cnt;
			QueryPerformanceCounter(&cnt);
			return cnt.QuadPart;
		}
#	elif defined(__linux__)
		static inline vi_tmTicks_t vi_tmGetTicks_impl(void)
		{	struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
			return 1000000000ULL * ts.tv_sec + ts.tv_nsec;
		}
#	else
#		error "You need to define function(s) for your OS and CPU"
#	endif
	
#	define vi_tmGetTicks vi_tmGetTicks_impl
#endif
// Definition of vi_tmGetTicks() function for different platforms. ^^^^^^^^^^^^

	// Main functions vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API void VI_TM_CALL vi_tmInit(VI_STD(size_t) reserve VI_DEFAULT(64));
	VI_TM_API void VI_TM_CALL vi_tmFinit();
	VI_NODISCARD VI_TM_API vi_tmAtomicTicks_t* VI_TM_CALL vi_tmItem(const char* name, VI_STD(size_t) cnt VI_DEFAULT(1));
	inline void vi_tmAdd(vi_tmAtomicTicks_t *amount, vi_tmTicks_t ticks)
	{	VI_STD(atomic_fetch_add_explicit)(amount, ticks, VI_MEMORY_ORDER(memory_order_relaxed));
	}
	VI_TM_API int VI_TM_CALL vi_tmResults(vi_tmLogRAW_t fn, void* data);
	VI_TM_API void VI_TM_CALL vi_tmClear(const char* name VI_DEFAULT(NULL));
	// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// Supporting functions. vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API void VI_TM_CALL vi_tmWarming(unsigned int threads VI_DEFAULT(0), unsigned int ms VI_DEFAULT(500));

	typedef struct vi_tmItem_t
	{	vi_tmAtomicTicks_t* amount_;
		vi_tmTicks_t start_; // Order matters!!! 'start_' must be initialized last!
	} vi_tmItem_t;

	VI_NODISCARD static inline vi_tmItem_t vi_tmStart(const char* name, VI_STD(size_t) amount VI_DEFAULT(1)) VI_NOEXCEPT
	{	vi_tmItem_t result;
		result.amount_ = vi_tmItem(name, amount);
		result.start_ = vi_tmGetTicks();
		return result;
	}

	static inline void vi_tmFinish(const vi_tmItem_t *itm) VI_NOEXCEPT
	{	const vi_tmTicks_t finish = vi_tmGetTicks();
		vi_tmAdd(itm->amount_, finish - itm->start_);
	}

	enum vi_tmReportFlags_e {
		vi_tmSortByTime = 0x00,
		vi_tmSortByName = 0x01,
		vi_tmSortBySpeed = 0x02,
		vi_tmSortByAmount = 0x03,
		vi_tmSortDescending = 0x00,
		vi_tmSortAscending = 0x08,
		vi_tmSortMask = 0x0F,

		vi_tmShowOverhead = 0x10,
		vi_tmShowUnit = 0x20,
		vi_tmShowDuration = 0x40,
		vi_tmShowMask = 0xF0,
	};
	VI_TM_API int VI_TM_CALL vi_tmReport
	(	vi_tmLogSTR_t callback VI_DEFAULT(reinterpret_cast<vi_tmLogSTR_t>(&std::fputs)),
		void* data VI_DEFAULT(stdout),
		int flags VI_DEFAULT(vi_tmSortByTime | vi_tmSortDescending)
	);

	enum vi_tmInfo_e
	{	VI_TM_INFO_VER,
		VI_TM_INFO_VERSION,
		VI_TM_INFO_TIME,
		VI_TM_BUILDTYPE,
	};
	VI_NODISCARD VI_TM_API const void* VI_TM_CALL vi_tmInfo(enum vi_tmInfo_e info VI_DEFAULT(VI_TM_INFO_VER));

	// Supporting functions. ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifdef __cplusplus
} // extern "C" {

namespace vi_tm
{
	class timer_t: public vi_tmItem_t
	{	timer_t(const timer_t&) = delete;
		timer_t& operator=(const timer_t&) = delete;
	public:
		timer_t(const char *name, std::size_t cnt = 1) noexcept
			: vi_tmItem_t{ vi_tmStart(name, cnt) }
		{
		}
		~timer_t() noexcept
		{	vi_tmFinish(this);
		}
	};

	class init_t
	{	std::string title_;
		vi_tmLogSTR_t cb_;
		void* data_;
		int flags_;
	public:
		init_t
		(	const char* title = "Timing report:\n",
			vi_tmLogSTR_t fn = reinterpret_cast<vi_tmLogSTR_t>(&std::fputs),
			void* data = stdout,
			int flags = vi_tmSortByTime | vi_tmSortDescending,
			std::size_t reserve = 64
		)
		: title_{ title }, cb_{ fn }, data_{ data }, flags_{ flags }
		{	vi_tmInit(reserve);
		}

		~init_t()
		{	if (!title_.empty())
			{	cb_(title_.c_str(), data_);
			}

			vi_tmReport(cb_, data_, flags_);
			vi_tmFinit();
		}
	};
} // namespace vi_tm {

#	if defined(VI_TM_DISABLE)
#		define VI_TM_INIT(...) int VI_MAKE_UNIC_ID(_vi_tm_dummy_){(__VA_ARGS__, 0)}
#		define VI_TM(...) int VI_MAKE_UNIC_ID(_vi_tm_dummy_){(__VA_ARGS__, 0)}
#		define VI_TM_REPORT(...) ((void)(__VA_ARGS__, 0))
#		define VI_TM_CLEAR ((void)0)
#		define VI_TM_INFO(f) NULL
#	else
#		define VI_TM_INIT(...) vi_tm::init_t VI_MAKE_UNIC_ID(_vi_tm_init_) {__VA_ARGS__}
#		define VI_TM(...) vi_tm::timer_t VI_MAKE_UNIC_ID(_vi_tm_variable_) {__VA_ARGS__}
#		define VI_TM_REPORT(...) vi_tmReport(__VA_ARGS__)
#		define VI_TM_CLEAR vi_tmClear(NULL)
#		define VI_TM_INFO(...) vi_tmInfo(__VA_ARGS__)
#	endif
#	define VI_TM_FUNC VI_TM( VI_FUNCNAME )

#endif // #ifdef __cplusplus

#endif // #ifndef VI_TIMING_VI_TIMING_H

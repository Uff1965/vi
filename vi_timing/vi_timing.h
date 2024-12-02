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

#	define VI_TM_VERSION_MAJOR 0	// 0 - 99
#	define VI_TM_VERSION_MINOR 13	// 0 - 999
#	define VI_TM_VERSION_PATCH 0	// 0 - 9999
#	define VI_TM_VERSION (((VI_TM_VERSION_MAJOR) * 1000U + (VI_TM_VERSION_MINOR)) * 10000U + (VI_TM_VERSION_PATCH))
#	define VI_TM_VERSION_STR VI_STR(VI_TM_VERSION_MAJOR) "." VI_STR(VI_TM_VERSION_MINOR) "." VI_STR(VI_TM_VERSION_PATCH)

#	if defined(_WIN32)
#		include <Windows.h>
#	elif defined(__linux__)
#		include <time.h> // for clock_gettime
#	endif

#	if defined(_M_X64) || defined(_M_AMD64) // MSVC on x86-64
#		include <intrin.h>
#		pragma intrinsic(__rdtscp, _mm_lfence)
#	elif defined(__x86_64__) || defined(__amd64__) // GCC on x86_64
#		include <x86intrin.h>
#	endif

#	ifdef __cplusplus
#		include <cassert>
#		include <cstdint>
#		include <cstdio>
#		include <string>
#	else
#		ifdef __STDC_NO_ATOMICS__
//			At the moment Atomics are available in Visual Studio 2022 with the /experimental:c11atomics flag.
//			"we left out support for some C11 optional features such as atomics" [Microsoft
//			https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#			error "Atomic objects and the atomic operation library are not supported."
#		endif
#		include <stdint.h>
#		include <stdio.h>
#	endif

#	include "common.h"

// Define VI_TM_CALL, VI_TM_API and VI_SYS_CALL vvvvvvvvvvvvvv
#	if defined(_WIN32) // Windows x86 or x64
#		define VI_SYS_CALL __cdecl
#		ifdef _WIN64
#			define VI_TM_CALL
#		else
#			define VI_TM_CALL __fastcall
#		endif

#		ifdef VI_TM_EXPORTS
#			define VI_TM_API __declspec(dllexport)
#		elif defined(VI_TM_SHARED)
#			define VI_TM_API __declspec(dllimport)

#			ifdef NDEBUG
#				pragma comment(lib, "vi_timing_s.lib")
#			else
#				pragma comment(lib, "vi_timing_sd.lib")
#			endif
#		else
#			define VI_TM_API

#			ifdef NDEBUG
#				pragma comment(lib, "vi_timing.lib")
#			else
#				pragma comment(lib, "vi_timing_d.lib")
#			endif
#		endif

#	elif defined(__ANDROID__)
#		define VI_TM_DISABLE "Android not supported yet."
#	elif defined (__linux__)
#		define VI_SYS_CALL
#		define VI_TM_CALL
#		ifdef VI_TM_EXPORTS
#			define VI_TM_API __attribute__((visibility("default")))
#		else
#			define VI_TM_API
#		endif
#	else
#		define VI_TM_DISABLE "Unknown platform!"
#	endif
// Define VI_TM_CALL and VI_TM_API ^^^^^^^^^^^^^^^^^^^^^^^

	typedef struct vi_tmInstance_t* VI_TM_HANDLE;
	typedef struct vi_tmTotal_t* VI_TM_HITEM;
	typedef VI_STD(uint64_t) vi_tmTicks_t;
	typedef int (*vi_tmLogRAW_t)(const char* name, vi_tmTicks_t time, VI_STD(size_t) amount, VI_STD(size_t) calls_cnt, void* data);

#	ifdef __cplusplus
extern "C" {
#	endif

// Definition of vi_tmGetTicks() function for different platforms. vvvvvvvvvvvv
#	ifndef vi_tmGetTicks
#		if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__amd64__) // MSC or GCC on Intel
			static inline vi_tmTicks_t vi_tmGetTicks_impl(void) VI_NOEXCEPT
			{	VI_STD(uint32_t) _; // Will be removed by the optimizer.
				const VI_STD(uint64_t) result = __rdtscp(&_);
				//	«If software requires RDTSCP to be executed prior to execution of any subsequent instruction 
				//	(including any memory accesses), it can execute LFENCE immediately after RDTSCP» - 
				//	(Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes:
				//	1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4. Vol. 2B. P.4-553)
				_mm_lfence();
				return result;
			}
#		elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
			static inline vi_tmTicks_t vi_tmGetTicks_impl(void) VI_NOEXCEPT
			{	VI_STD(uint64_t) result;
				__asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(result));
				return result;
			}
#		elif defined(_WIN32) // Windows
			static inline vi_tmTicks_t vi_tmGetTicks_impl(void) VI_NOEXCEPT
			{	LARGE_INTEGER cnt;
				QueryPerformanceCounter(&cnt);
				return cnt.QuadPart;
			}
#		elif defined(__linux__)
			static inline vi_tmTicks_t vi_tmGetTicks_impl(void) VI_NOEXCEPT
			{	struct timespec ts;
				clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
				return 1000000000ULL * ts.tv_sec + ts.tv_nsec;
			}
#		else
#			error "You need to define function(s) for your OS and CPU"
#		endif
		
#		define vi_tmGetTicks vi_tmGetTicks_impl
#	endif
// Definition of vi_tmGetTicks() function for different platforms. ^^^^^^^^^^^^

	// Main functions vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	enum vi_tmInfo_e
	{	VI_TM_INFO_VER,
		VI_TM_INFO_VERSION,
		VI_TM_INFO_BUILDTYPE,
		VI_TM_INFO_BUILDNUMBER,
		VI_TM_INFO_BUILDTIME,
	};

	VI_TM_API VI_NODISCARD VI_STD(uintptr_t) VI_TM_CALL vi_tmInfo(enum vi_tmInfo_e info VI_DEFAULT(VI_TM_INFO_VER));
	VI_TM_API void VI_TM_CALL vi_tmInit();
	VI_TM_API VI_NODISCARD VI_TM_HANDLE VI_TM_CALL vi_tmCreate();
	VI_TM_API void VI_TM_CALL vi_tmAdd(VI_TM_HANDLE h,const char *name,  vi_tmTicks_t ticks, VI_STD(size_t) amount VI_DEFAULT(1)) VI_NOEXCEPT;
	VI_TM_API int VI_TM_CALL vi_tmResults(VI_TM_HANDLE h, vi_tmLogRAW_t fn, void* data);
	VI_TM_API void VI_TM_CALL vi_tmClear(VI_TM_HANDLE h, const char* name VI_DEFAULT(NULL)) VI_NOEXCEPT;
	VI_TM_API void VI_TM_CALL vi_tmClose(VI_TM_HANDLE h);
	VI_TM_API void VI_TM_CALL vi_tmFinit(void);
	// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// Supporting functions. vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API void VI_TM_CALL vi_tm_Warming(unsigned threads VI_DEFAULT(0), unsigned ms VI_DEFAULT(500));
	static inline int VI_SYS_CALL vi_tm_ReportCallback(const char* str, void* data)
	{	return VI_STD(fputs)(str, VI_R_CAST(VI_STD(FILE)*, data));
	}

	typedef int (VI_SYS_CALL *vi_tmLogSTR_t)(const char* str, void* data); // Must be compatible with std::fputs!
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
		vi_tmShowNoHeader = 0x80,
		vi_tmShowMask = 0xF0,
	};
	VI_TM_API int VI_TM_CALL vi_tmReport
	(	VI_TM_HANDLE h,
		int flags VI_DEFAULT(vi_tmSortByTime | vi_tmSortDescending),
		vi_tmLogSTR_t callback VI_DEFAULT(vi_tm_ReportCallback),
		void* data VI_DEFAULT(stdout)
	);

	// Supporting functions. ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
} // extern "C" {

namespace vi_tm
{
	class timer_t
	{	const VI_TM_HANDLE h_ = nullptr;
		const char *name_;
		const std::size_t cnt_;
		const vi_tmTicks_t start_{ vi_tmGetTicks() }; // Order matters!!! 'start_' must be initialized last!

		timer_t(const timer_t&) = delete;
		timer_t& operator=(const timer_t&) = delete;
	public:
		timer_t(VI_TM_HANDLE h, const char *name, std::size_t cnt = 1) noexcept
			: h_{ h }, name_{ name }, cnt_{ cnt }
		{
		}
		~timer_t() noexcept
		{	const auto finish = vi_tmGetTicks();
			vi_tmAdd(h_, name_, finish - start_, cnt_);
		}
	};

	class init_t
	{	static constexpr auto default_cb = &vi_tm_ReportCallback;
		static inline const auto default_data = reinterpret_cast<void*>(stdout);
		std::string title_ = "Timing report:\n";
		vi_tmLogSTR_t cb_ = default_cb;
		void* data_ = default_data;
		int flags_ = 0;
		init_t(const init_t &) = delete;
		init_t &operator=(const init_t &) = delete;
	public:
		template<typename... Args> init_t(Args&&... args);
		~init_t();
		template<typename T, typename... Args> void init(T &&v, Args&&... args);
		void init();
	};

	inline void init_t::init()
	{	vi_tmInit();
	}
	template<typename T, typename... Args>
	inline void init_t::init(T &&v, Args&&... args)
	{
		if constexpr (std::is_same_v<vi_tmReportFlags_e, std::decay_t<T>>)
		{	flags_ |= v;
		}
		else if constexpr (std::is_same_v<vi_tmLogSTR_t, std::decay_t<T>>)
		{	assert(default_cb == cb_ && nullptr != v);
			cb_ = v;
		}
		else if constexpr (std::is_pointer_v<T>)
		{	assert(default_data == data_);
			data_ = v;
		}
		else if constexpr (std::is_convertible_v<T, decltype(title_)>)
		{	title_ = v;
		}
		else
		{	assert(false);
		}

		init(std::forward<Args>(args)...);
	}

	template<typename... Args>
	inline init_t::init_t(Args&&... args)
	{	init(std::forward<Args>(args)...);
	}

	inline init_t::~init_t()
	{	if (!title_.empty())
		{	cb_(title_.c_str(), data_);
		}

		vi_tmReport(nullptr, flags_, cb_, data_);
		vi_tmFinit();
	}

} // namespace vi_tm {

#		if defined(VI_TM_DISABLE)
#			define VI_TM_INIT(...) int VI_MAKE_ID(_vi_tm_dummy_){(__VA_ARGS__, 0)}
#			define VI_TM(...) int VI_MAKE_ID(_vi_tm_dummy_){(__VA_ARGS__, 0)}
#			define VI_TM_REPORT(...) ((void)(__VA_ARGS__, 0))
#			define VI_TM_CLEAR ((void)0)
#			define VI_TM_INFO(f) NULL
#		else
#			define VI_TM_INIT(...) vi_tm::init_t VI_MAKE_ID(_vi_tm_init_) {__VA_ARGS__}
#			define VI_TM(...) vi_tm::timer_t VI_MAKE_ID(_vi_tm_variable_) {NULL, __VA_ARGS__}
#			define VI_TM_REPORT(...) vi_tmReport(NULL, __VA_ARGS__)
#			define VI_TM_CLEAR vi_tmClear(NULL, NULL)
#			define VI_TM_INFO(...) vi_tmInfo(__VA_ARGS__)
#		endif
#		define VI_TM_FUNC VI_TM( VI_FUNCNAME )

#	endif // #ifdef __cplusplus

#endif // #ifndef VI_TIMING_VI_TIMING_H

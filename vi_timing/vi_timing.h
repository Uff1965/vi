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
#	define VI_TM_VERSION_MINOR 3	// 0 - 999
#	define VI_TM_VERSION_PATCH 1	// 0 - 9999
#	define VI_TM_VERSION (((VI_TM_VERSION_MAJOR) * 1000U + (VI_TM_VERSION_MINOR)) * 10000U + (VI_TM_VERSION_PATCH))
#	define VI_TM_VERSION_STR VI_STR(VI_TM_VERSION_MAJOR) "." VI_STR(VI_TM_VERSION_MINOR) "." VI_STR(VI_TM_VERSION_PATCH)

#	ifdef __cplusplus
#		include <cassert>
#		include <cstdint>
#		include <cstdio> // For fputs and stdout
#		include <string>
#		include <utility>
#	else
#		include <stdint.h>
#		include <stdio.h> // For fputs and stdout
#	endif

#	include "common.h"

// Define: VI_TM_CALL, VI_TM_API and VI_SYS_CALL vvvvvvvvvvvvvv
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
// Define: VI_TM_CALL, VI_TM_API and VI_SYS_CALL ^^^^^^^^^^^^^^^^^^^^^^^

typedef struct vi_tmInstance_t* VI_TM_HBOOK;
typedef struct vi_tm_journal_t *VI_TM_HSHEET;
typedef VI_STD(uint64_t) vi_tmTicks_t;
typedef int (*vi_tmLogRAW_t)(const char* name, vi_tmTicks_t time, VI_STD(size_t) amount, VI_STD(size_t) calls_cnt, void* data);
enum vi_tmInfo_e
{	VI_TM_INFO_VER,
	VI_TM_INFO_VERSION,
	VI_TM_INFO_BUILDNUMBER,
	VI_TM_INFO_BUILDTYPE,
};

#	ifdef __cplusplus
extern "C"
{
#	endif

// Main functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API VI_NODISCARD VI_STD(uintptr_t) VI_TM_CALL vi_tmInfo(enum vi_tmInfo_e info);
	VI_TM_API VI_NODISCARD int VI_TM_CALL vi_tmInit(void); // If successful, returns 0.
	VI_TM_API VI_NODISCARD VI_TM_HBOOK VI_TM_CALL vi_tmBookCreate(void);
	VI_TM_API vi_tmTicks_t VI_TM_CALL vi_tmClock(void) VI_NOEXCEPT;
	VI_TM_API VI_TM_HSHEET VI_TM_CALL vi_tmSheet(VI_TM_HBOOK h, const char* name);
	VI_TM_API void VI_TM_CALL vi_tmRecord(VI_TM_HSHEET j, vi_tmTicks_t tick_diff, VI_STD(size_t) amount VI_DEF(1)) VI_NOEXCEPT;
	VI_TM_API int VI_TM_CALL vi_tmResult(VI_TM_HBOOK h, const char* name, vi_tmTicks_t *ticks, VI_STD(size_t) *amount, VI_STD(size_t) *calls_cnt);
	VI_TM_API int VI_TM_CALL vi_tmResults(VI_TM_HBOOK h, vi_tmLogRAW_t fn, void* data);
	VI_TM_API void VI_TM_CALL vi_tmBookClear(VI_TM_HBOOK h, const char* name VI_DEF(NULL)) VI_NOEXCEPT;
	VI_TM_API void VI_TM_CALL vi_tmBookClose(VI_TM_HBOOK h);
	VI_TM_API void VI_TM_CALL vi_tmFinit(void);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Supporting functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	typedef int (VI_SYS_CALL *vi_tmLogSTR_t)(const char* str, void* data); // Must be compatible with std::fputs!
	enum vi_tmReportFlags_e
	{	vi_tmSortByTime = 0x00,
		vi_tmSortByName = 0x01,
		vi_tmSortBySpeed = 0x02,
		vi_tmSortByAmount = 0x03,
		vi_tmSortMask = 0x07,
		vi_tmSortDescending = 0x00,
		vi_tmSortAscending = 0x08,

		vi_tmShowOverhead = 0x10,
		vi_tmShowUnit = 0x20,
		vi_tmShowDuration = 0x40,
		vi_tmShowResolution = 0x80,
		vi_tmShowNoHeader = 0x0100,
	};

	static inline int VI_SYS_CALL vi_tmReportCallback(const char* str, void* data)
	{	return VI_STD(fputs)(str, VI_R_CAST(VI_STD(FILE)*, data));
	}
	VI_TM_API int VI_TM_CALL vi_tmReport(VI_TM_HBOOK h, unsigned flags VI_DEF(0), vi_tmLogSTR_t callback VI_DEF(vi_tmReportCallback), void *data VI_DEF(stdout));
	VI_TM_API void VI_TM_CALL vi_tmWarming(unsigned threads VI_DEF(0), unsigned ms VI_DEF(500));
// Supporting functions: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
}
#	endif

// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#	ifdef __cplusplus
namespace vi_tm
{
	class init_t
	{	static constexpr auto default_cb = &vi_tmReportCallback;
		static inline const auto default_data = reinterpret_cast<void*>(stdout);
		std::string title_ = "Timing report:\n";
		vi_tmLogSTR_t cb_ = default_cb;
		void* data_ = default_data;
		unsigned flags_ = 0;
		init_t(const init_t &) = delete;
		init_t &operator=(const init_t &) = delete;
	public:
		template<typename... Args>
		explicit init_t(Args&&... args);
		~init_t();
		template<typename T, typename... Args> void init(T &&v, Args&&... args);
		void init();
	};

	inline void init_t::init()
	{	[[maybe_unused]] const auto result = vi_tmInit();
		assert(0 == result);
	}
	template<typename T, typename... Args>
	void init_t::init(T &&v, Args&&... args)
	{
		if constexpr (std::is_same_v<vi_tmReportFlags_e, std::decay_t<T>>)
		{	flags_ |= v;
		}
		else if constexpr (std::is_same_v<vi_tmLogSTR_t, std::decay_t<T>>)
		{	assert(default_cb == cb_ && nullptr != v);
			cb_ = v;
		}
		else if constexpr (std::is_same_v<T, decltype(title_)>)
		{	title_ = std::forward<T>(v);
		}
		else if constexpr (std::is_convertible_v<T, decltype(title_)>)
		{	title_ = v;
		}
		else if constexpr (std::is_pointer_v<T>)
		{	assert(default_data == data_);
			data_ = v;
		}
		else
		{	assert(false);
		}

		init(std::forward<Args>(args)...);
	}

	template<typename... Args>
	init_t::init_t(Args&&... args)
	{	init(std::forward<Args>(args)...);
	}

	inline init_t::~init_t()
	{	if (!title_.empty())
		{	cb_(title_.c_str(), data_);
		}

		vi_tmReport(nullptr, flags_, cb_, data_);
		vi_tmFinit();
	}

	class meter_t
	{	VI_TM_HSHEET h_;
		std::size_t amt_;
		const vi_tmTicks_t start_ = vi_tmClock(); // Order matters!!! 'start_' must be initialized last!
		meter_t(const meter_t &) = delete;
		void operator=(const meter_t &) = delete;
	public:
		meter_t(VI_TM_HSHEET h, std::size_t amt = 1): h_{h}, amt_{amt} {/**/}
		~meter_t() { const auto finish = vi_tmClock(); vi_tmRecord(h_, finish - start_, amt_); }
	};
} // namespace vi_tm

#		if defined(VI_TM_DISABLE)
#			define VI_TM_INIT(...) static const int VI_MAKE_ID(_vi_tm_) = 0
#			define VI_TM(...) static const int VI_MAKE_ID(_vi_tm_) = 0
#			define VI_TM_FUNC ((void)0)
#			define VI_TM_REPORT(...) ((void)0)
#			define VI_TM_CLEAR ((void)0)
#			define VI_TM_FULLVERSION ""
#		else
#			define VI_TM_INIT(...) vi_tm::init_t VI_MAKE_ID(_vi_tm_) {__VA_ARGS__}
#			define VI_TM(...)\
				const auto VI_MAKE_ID(_vi_tm_) = [](const char *n, std::size_t a = 1)\
					{	static VI_TM_HSHEET const h = vi_tmSheet(nullptr, n);\
						return vi_tm::meter_t{h, a};\
					}(__VA_ARGS__)
#			define VI_TM_FUNC VI_TM( VI_FUNCNAME )
#			define VI_TM_REPORT(...) vi_tmReport(nullptr, __VA_ARGS__)
#			define VI_TM_CLEAR(...) vi_tmBookClear(nullptr, __VA_ARGS__)
#			define VI_TM_FULLVERSION reinterpret_cast<const char*>(vi_tmInfo(VI_TM_INFO_VERSION))
#		endif

#	endif // #ifdef __cplusplus
// Auxiliary macros: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#endif // #ifndef VI_TIMING_VI_TIMING_H

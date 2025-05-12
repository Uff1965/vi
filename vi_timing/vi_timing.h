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
#	define VI_TM_VERSION_MINOR 1	// 0 - 999
#	define VI_TM_VERSION_PATCH 0	// 0 - 9999

#	include <stdint.h>
#	include <stdio.h> // For fputs and stdout

#ifdef __cplusplus
#	include <cassert>
#	include <string>

#	define VI_NODISCARD [[nodiscard]]
#	define VI_NOEXCEPT noexcept
#	define VI_DEF(v) = (v)
#else
#	define VI_NODISCARD
#	define VI_NOEXCEPT
#	define VI_DEF(v)
#endif

// Define: VI_FUNCNAME, VI_SYS_CALL, VI_TM_CALL and VI_TM_API vvvvvvvvvvvvvv
#	if defined(_MSC_VER)
#		define VI_FUNCNAME __FUNCSIG__

#		ifdef _M_IX86
#			define VI_SYS_CALL __cdecl
#			define VI_TM_CALL __fastcall
#		else
#			define VI_SYS_CALL
#			define VI_TM_CALL
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
#	elif defined (__GNUC__) || defined(__clang__)
#		define VI_FUNCNAME __PRETTY_FUNCTION__

#		ifdef __i386__
#			define VI_SYS_CALL __attribute__((cdecl))
#			define VI_TM_CALL __attribute__((fastcall))
#		else
#			define VI_SYS_CALL
#			define VI_TM_CALL
#		endif

#		ifdef VI_TM_EXPORTS
#			define VI_TM_API __attribute__((visibility("default")))
#		else
#			define VI_TM_API
#		endif
#	else
#		define VI_FUNCNAME __func__
#		define VI_TM_DISABLE "Unknown compiler!"
#	endif
// Define: VI_FUNCNAME, VI_SYS_CALL, VI_TM_CALL and VI_TM_API ^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#define VI_STR_AUX(s) #s
#define VI_STR(s) VI_STR_AUX(s)
#define VI_STR_GUM_AUX( a, b ) a##b
#define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#define VI_MAKE_ID( prefix ) VI_STR_GUM( prefix, __LINE__ )

#if defined(_MSC_VER)
#	define VI_OPTIMIZE_OFF	_Pragma("optimize(\"\", off)")
#	define VI_OPTIMIZE_ON	_Pragma("optimize(\"\", on)")
#elif defined(__clang__)
#	define VI_OPTIMIZE_OFF	_Pragma("clang optimize push") \
							_Pragma("clang optimize off")
#	define VI_OPTIMIZE_ON	_Pragma("clang optimize pop")
#elif defined(__GNUC__)
#	define VI_OPTIMIZE_OFF	_Pragma("GCC push_options") \
							_Pragma("GCC optimize(\"O0\")")
#	define VI_OPTIMIZE_ON	_Pragma("GCC pop_options")
#else
#	define VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_ON
#endif
// Auxiliary macros: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

typedef uint64_t VI_TM_TICK;
typedef uint64_t VI_TM_TDIFF;
typedef struct vi_tmJournal_t *VI_TM_HJOUR;
typedef struct vi_tmMeasuring_t *VI_TM_HMEAS;
typedef int (VI_TM_CALL *vi_tmMeasuringEnumCallback_t)(VI_TM_HMEAS meas, void* data);
// Enumeration for various timing information types.
typedef enum
{	VI_TM_INFO_VER,         // unsigned: Version number of the library.
	VI_TM_INFO_VERSION,     // const char*: Full version string of the library.
	VI_TM_INFO_BUILDNUMBER, // unsigned: Build number of the library.
	VI_TM_INFO_BUILDTYPE,   // const char*: Build type, either "Release" or "Debug".
	VI_TM_INFO_RESOLUTION,  // const double*: Clock resolution in ticks.
	VI_TM_INFO_DURATION,    // const double*: Measure duration in seconds.
	VI_TM_INFO_OVERHEAD,    // const double*: Clock overhead in ticks.
	VI_TM_INFO_UNIT,        // const double*: Seconds per tick (time unit).
} vi_tmInfo_e;

#	ifdef __cplusplus
extern "C"
{
#	endif

// Main functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API VI_NODISCARD VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) VI_NOEXCEPT;
	VI_TM_API int VI_TM_CALL vi_tmInit(void); // If successful, returns 0.
	VI_TM_API void VI_TM_CALL vi_tmFinit(void);
	VI_TM_API VI_NODISCARD VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate(void);
	VI_TM_API void VI_TM_CALL vi_tmJournalClear(VI_TM_HJOUR j, const char* name VI_DEF(NULL)) VI_NOEXCEPT;
	VI_TM_API void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOUR j);
	VI_TM_API VI_NODISCARD VI_TM_HMEAS VI_TM_CALL vi_tmMeasuring(VI_TM_HJOUR j, const char* name);
	VI_TM_API int VI_TM_CALL vi_tmMeasuringEnumerate(VI_TM_HJOUR j, vi_tmMeasuringEnumCallback_t fn, void* data);
	VI_TM_API void VI_TM_CALL vi_tmMeasuringAdd(VI_TM_HMEAS m, VI_TM_TDIFF duration, size_t amount VI_DEF(1)) VI_NOEXCEPT;
	VI_TM_API void VI_TM_CALL vi_tmMeasuringGet(VI_TM_HMEAS m, const char **name, VI_TM_TDIFF *total, size_t *amt, size_t *calls);
	VI_TM_API void VI_TM_CALL vi_tmMeasuringClear(VI_TM_HMEAS m);
	VI_TM_API VI_NODISCARD uintptr_t VI_TM_CALL vi_tmInfo(vi_tmInfo_e info);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	typedef int (VI_SYS_CALL *vi_tmRptCb_t)(const char* str, void* data); // ABI must be compatible with std::fputs!
	typedef enum
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
	} vi_tmReportFlags_e;

	static inline int VI_SYS_CALL vi_tmRptCb(const char* str, void* data)
	{	return fputs(str, (FILE*)data);
	}
	VI_TM_API int VI_TM_CALL vi_tmReport(VI_TM_HJOUR j, unsigned flags VI_DEF(0), vi_tmRptCb_t VI_DEF(vi_tmRptCb), void* VI_DEF(stdout));
	VI_TM_API void VI_TM_CALL vi_tmWarming(unsigned threads VI_DEF(0), unsigned ms VI_DEF(500));
	VI_TM_API void VI_TM_CALL vi_tmCurrentThreadAffinityFixate(void);
	VI_TM_API void VI_TM_CALL vi_tmCurrentThreadAffinityRestore(void);
	VI_TM_API void VI_TM_CALL vi_tmThreadYield(void);
// Auxiliary functions: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
} // extern "C"
#	endif

#	ifdef __cplusplus
namespace vi_tm
{
	class init_t
	{	static constexpr auto default_callback_fn = &vi_tmRptCb;
		static inline const auto default_callback_data = static_cast<void*>(stdout);

		std::string title_ = "Timing report:\n";
		vi_tmRptCb_t callback_function_ = default_callback_fn;
		void* callback_data_ = default_callback_data;
		unsigned flags_ = 0;

		init_t(const init_t &) = delete;
		init_t(init_t &&) = delete;
		init_t &operator=(const init_t &) = delete;
		init_t &operator=(init_t &&) = delete;
	public:
		template<typename... Args> explicit init_t(Args&&... args)
		{	init(std::forward<Args>(args)...);
		}
		~init_t()
		{	if (!title_.empty())
			{	callback_function_(title_.c_str(), callback_data_);
			}

			vi_tmReport(nullptr, flags_, callback_function_, callback_data_);
			vi_tmFinit();
		}
		template<typename T, typename... Args> void init(T &&v, Args&&... args)
		{	if constexpr (std::is_same_v<std::decay_t<T>, vi_tmReportFlags_e>)
			{	flags_ |= v;
			}
			else if constexpr (std::is_same_v<std::decay_t<T>, vi_tmRptCb_t>)
			{	assert(default_callback_fn == callback_function_ && nullptr != v);
				callback_function_ = v;
			}
			else if constexpr (std::is_same_v<T, decltype(title_)>)
			{	title_ = std::forward<T>(v);
			}
			else if constexpr (std::is_convertible_v<T, decltype(title_)>)
			{	title_ = v;
			}
			else if constexpr (std::is_pointer_v<T>)
			{	assert(default_callback_data == callback_data_);
				callback_data_ = v;
			}
			else
			{	assert(false); // Unknown parameter type.
			}

			init(std::forward<Args>(args)...);
		}
		void init() const
		{	[[maybe_unused]] const auto result = vi_tmInit();
			assert(0 == result);
		}
	}; // class init_t

	class measurer_t
	{	VI_TM_HMEAS meas_;
		size_t amt_;
		const VI_TM_TICK start_ = vi_tmGetTicks(); // Order matters!!! 'start_' must be initialized last!
		measurer_t(const measurer_t &) = delete;
		void operator=(const measurer_t &) = delete;
	public:
		measurer_t(VI_TM_HMEAS m, size_t amt = 1): meas_{m}, amt_{amt} {/**/}
		~measurer_t() { const auto finish = vi_tmGetTicks(); vi_tmMeasuringAdd(meas_, finish - start_, amt_); }
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
				const auto VI_MAKE_ID(_vi_tm_) = [](const char *name, size_t amount = 1)\
					{	static auto const h = vi_tmMeasuring(nullptr, name);\
						return vi_tm::measurer_t{h, amount};\
					}(__VA_ARGS__)
#			define VI_TM_FUNC VI_TM( VI_FUNCNAME )
#			define VI_TM_REPORT(...) vi_tmReport(nullptr, __VA_ARGS__)
#			define VI_TM_CLEAR(...) vi_tmJournalClear(nullptr, __VA_ARGS__)
#			define VI_TM_FULLVERSION reinterpret_cast<const char*>(vi_tmInfo(VI_TM_INFO_VERSION))
#		endif

#	endif // #ifdef __cplusplus
#endif // #ifndef VI_TIMING_VI_TIMING_H

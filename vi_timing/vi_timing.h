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

#	ifdef __cplusplus
#		include <cassert>
#		include <cstdint>
#		include <cstdio> // For fputs and stdout
#		include <string>
#	else
#		include <stdint.h>
#		include <stdio.h> // For fputs and stdout
#	endif

#ifdef __cplusplus
#	define VI_CAST_C(T, s) const_cast<T>(s)
#	define VI_CAST_R(T, s) reinterpret_cast<T>(s)
#	define VI_CAST_S(T, s) static_cast<T>(s)
#	define VI_DEF(v) = (v)
#	define VI_NODISCARD [[nodiscard]]
#	define VI_NOEXCEPT noexcept
#	define VI_STD(s) std::s
#else
#	define VI_CAST_C(T, s) (T)(s)
#	define VI_CAST_R(T, s) (T)(s)
#	define VI_CAST_S(T, s) (T)(s)
#	define VI_DEF(v)
#	define VI_NODISCARD
#	define VI_NOEXCEPT
#	define VI_STD(s) s
#endif

// Define: VI_TM_CALL, VI_TM_API and VI_SYS_CALL vvvvvvvvvvvvvv
#	if defined(_MSC_VER)
#		ifdef __i386__
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
#		define VI_TM_DISABLE "Unknown compiler!"
#	endif
// Define: VI_TM_CALL, VI_TM_API and VI_SYS_CALL ^^^^^^^^^^^^^^^^^^^^^^^

typedef struct vi_tmJournal_t* VI_TM_HJOURNAL;
typedef struct vi_tmMeasPoint_t *VI_TM_HMEASPOINT;
typedef VI_STD(uint64_t) VI_TM_TICK;
typedef int (VI_TM_CALL *vi_tmLogRAW_t)(const char* name, VI_TM_TICK total, VI_STD(size_t) amount, VI_STD(size_t) calls_cnt, void* data);
enum vi_tmInfo_e
{	VI_TM_INFO_VER, // unsigned
	VI_TM_INFO_VERSION, // const char*
	VI_TM_INFO_BUILDNUMBER, // unsigned
	VI_TM_INFO_BUILDTYPE, // "Release" or "Debug"
	VI_TM_INFO_RESOLUTION, // const double* - Clock resolution [ticks]
	VI_TM_INFO_DURATION, // const double* - Measure duration [sec]
	VI_TM_INFO_OVERHEAD, // const double* - Clock duration [ticks]
	VI_TM_INFO_UNIT, // const double* - seconds per tick [sec]
};

#	ifdef __cplusplus
extern "C"
{
#	endif

// Main functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API VI_NODISCARD VI_STD(uintptr_t) VI_TM_CALL vi_tmInfo(enum vi_tmInfo_e info);
	VI_TM_API VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) VI_NOEXCEPT;
	VI_TM_API int VI_TM_CALL vi_tmInit(void); // If successful, returns 0.
	VI_TM_API void VI_TM_CALL vi_tmFinit(void);
	VI_TM_API VI_NODISCARD VI_TM_HJOURNAL VI_TM_CALL vi_tmJournalCreate(void);
	VI_TM_API void VI_TM_CALL vi_tmJournalClear(VI_TM_HJOURNAL h, const char* name VI_DEF(NULL)) VI_NOEXCEPT;
	VI_TM_API void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOURNAL h);
	VI_TM_API VI_TM_HMEASPOINT VI_TM_CALL vi_tmMeasPoint(VI_TM_HJOURNAL h, const char* name);
	VI_TM_API void VI_TM_CALL vi_tmMeasPointAdd(VI_TM_HMEASPOINT j, VI_TM_TICK duration, VI_STD(size_t) amount VI_DEF(1)) VI_NOEXCEPT;
	VI_TM_API int VI_TM_CALL vi_tmResult(VI_TM_HJOURNAL h, const char* name, VI_TM_TICK *total, VI_STD(size_t) *amount, VI_STD(size_t) *calls_cnt);
	VI_TM_API int VI_TM_CALL vi_tmResults(VI_TM_HJOURNAL h, vi_tmLogRAW_t fn, void* data);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
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
	{	return VI_STD(fputs)(str, VI_CAST_R(VI_STD(FILE)*, data));
	}
	VI_TM_API int VI_TM_CALL vi_tmReport(VI_TM_HJOURNAL h, unsigned flags VI_DEF(0), vi_tmLogSTR_t callback VI_DEF(vi_tmReportCallback), void *data VI_DEF(stdout));
	VI_TM_API void VI_TM_CALL vi_tmWarming(unsigned threads VI_DEF(0), unsigned ms VI_DEF(500));
	VI_TM_API void VI_TM_CALL vi_tmThreadAffinityFixate(void);
	VI_TM_API void VI_TM_CALL vi_tmThreadAffinityRestore(void);
	VI_TM_API void VI_TM_CALL vi_tmThreadYield(void);
	static inline void vi_tmThreadPrepare(void)
	{	vi_tmThreadAffinityFixate();
		vi_tmWarming(1, 500);
		vi_tmThreadYield();
	}
// Auxiliary functions: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
} // extern "C"
#	endif

// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#define VI_STR_AUX(s) #s
#define VI_STR(s) VI_STR_AUX(s)
#define VI_STR_GUM_AUX( a, b ) a##b
#define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#define VI_MAKE_ID( prefix ) VI_STR_GUM( prefix, __LINE__ )

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
#endif

#	ifdef __cplusplus
namespace vi_tm
{
	class init_t
	{	static constexpr auto default_callback_fn = &vi_tmReportCallback;
		static inline const auto default_callback_data = static_cast<void*>(stdout);
		std::string title_ = "Timing report:\n";
		vi_tmLogSTR_t callback_function_ = default_callback_fn;
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
			else if constexpr (std::is_same_v<std::decay_t<T>, vi_tmLogSTR_t>)
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

	class meter_t
	{	VI_TM_HMEASPOINT h_;
		std::size_t amt_;
		const VI_TM_TICK start_ = vi_tmGetTicks(); // Order matters!!! 'start_' must be initialized last!
		meter_t(const meter_t &) = delete;
		void operator=(const meter_t &) = delete;
	public:
		meter_t(VI_TM_HMEASPOINT h, std::size_t amt = 1): h_{h}, amt_{amt} {/**/}
		~meter_t() { const auto finish = vi_tmGetTicks(); vi_tmMeasPointAdd(h_, finish - start_, amt_); }
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
				const auto VI_MAKE_ID(_vi_tm_) = [](const char *name, std::size_t amount = 1)\
					{	static auto const h = vi_tmMeasPoint(nullptr, name);\
						return vi_tm::meter_t{h, amount};\
					}(__VA_ARGS__)
#			define VI_TM_FUNC VI_TM( VI_FUNCNAME )
#			define VI_TM_REPORT(...) vi_tmReport(nullptr, __VA_ARGS__)
#			define VI_TM_CLEAR(...) vi_tmJournalClear(nullptr, __VA_ARGS__)
#			define VI_TM_FULLVERSION reinterpret_cast<const char*>(vi_tmInfo(VI_TM_INFO_VERSION))
#		endif

#	endif // #ifdef __cplusplus
// Auxiliary macros: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#endif // #ifndef VI_TIMING_VI_TIMING_H

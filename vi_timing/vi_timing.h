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
#		define VI_TM_DISABLE "Unknown compiler!"
#		define VI_FUNCNAME __func__
#		define VI_SYS_CALL
#		define VI_TM_CALL
#		define VI_TM_API
#	endif
// Define: VI_FUNCNAME, VI_SYS_CALL, VI_TM_CALL and VI_TM_API ^^^^^^^^^^^^^^^^^^^^^^^

typedef uint64_t VI_TM_TICK;
typedef uint64_t VI_TM_TDIFF;
typedef struct vi_tmJournal_t *VI_TM_HJOUR;
typedef struct vi_tmMeasuring_t *VI_TM_HMEAS;
typedef int (VI_TM_CALL *vi_tmMeasuringEnumCallback_t)(VI_TM_HMEAS meas, void* data); // Returning a non-zero value aborts the enumeration.

#	ifdef __cplusplus
extern "C" {
#	endif
// Main functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API VI_NODISCARD VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) VI_NOEXCEPT;
	VI_TM_API int VI_TM_CALL vi_tmInit(void); // If successful, returns 0.
	VI_TM_API void VI_TM_CALL vi_tmFinit(void);
	VI_TM_API VI_NODISCARD VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate(void);
	VI_TM_API void VI_TM_CALL vi_tmJournalReset(VI_TM_HJOUR j, const char* name VI_DEF(NULL)) VI_NOEXCEPT;
	VI_TM_API void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOUR j);
	VI_TM_API VI_NODISCARD VI_TM_HMEAS VI_TM_CALL vi_tmMeasuring(VI_TM_HJOUR j, const char* name);
	VI_TM_API int VI_TM_CALL vi_tmMeasuringEnumerate(VI_TM_HJOUR j, vi_tmMeasuringEnumCallback_t fn, void* data);
	VI_TM_API void VI_TM_CALL vi_tmMeasuringAdd(VI_TM_HMEAS m, VI_TM_TDIFF duration, size_t amount VI_DEF(1)) VI_NOEXCEPT;
	VI_TM_API void VI_TM_CALL vi_tmMeasuringGet(VI_TM_HMEAS m, const char **name, VI_TM_TDIFF *total, size_t *amt, size_t *calls);
	VI_TM_API void VI_TM_CALL vi_tmMeasuringReset(VI_TM_HMEAS m);

	typedef enum // Enumeration for various timing information types.
	{	VI_TM_INFO_VER,         // unsigned*: Version number of the library.
		VI_TM_INFO_BUILDNUMBER, // unsigned*: Build number of the library.
		VI_TM_INFO_VERSION,     // const char*: Full version string of the library.
		VI_TM_INFO_BUILDTYPE,   // const char*: Build type, either "Release" or "Debug".
		VI_TM_INFO_LIBRARYTYPE, // const char*: Library type, either "Shared" or "Static".
		VI_TM_INFO_RESOLUTION,  // const double*: Clock resolution in ticks.
		VI_TM_INFO_DURATION,    // const double*: Measure duration in seconds.
		VI_TM_INFO_OVERHEAD,    // const double*: Clock overhead in ticks.
		VI_TM_INFO_UNIT,        // const double*: Seconds per tick (time unit).

		VI_TM_INFO__COUNT,      // Number of information types.
	} vi_tmInfo_e;
	VI_TM_API VI_NODISCARD const void* VI_TM_CALL vi_tmStaticInfo(vi_tmInfo_e info);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	typedef int (VI_SYS_CALL *vi_tmRptCb_t)(const char* str, void* data); // ABI must be compatible with std::fputs!
	static inline int VI_SYS_CALL vi_tmRptCb(const char* str, void* data)
	{	return fputs(str, (FILE*)data);
	}
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
	VI_TM_API int VI_TM_CALL vi_tmReport(VI_TM_HJOUR j, unsigned flags VI_DEF(0), vi_tmRptCb_t VI_DEF(vi_tmRptCb), void* VI_DEF(stdout));
	VI_TM_API void VI_TM_CALL vi_tmWarming(unsigned threads VI_DEF(0), unsigned ms VI_DEF(500));
	VI_TM_API void VI_TM_CALL vi_tmCurrentThreadAffinityFixate(void);
	VI_TM_API void VI_TM_CALL vi_tmCurrentThreadAffinityRestore(void);
	VI_TM_API void VI_TM_CALL vi_tmThreadYield(void);
// Auxiliary functions: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
} // extern "C"
#	endif
#endif // #ifndef VI_TIMING_VI_TIMING_H

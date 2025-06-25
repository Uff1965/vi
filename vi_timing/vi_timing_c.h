/********************************************************************\
'vi_timing' is a compact and lightweight library for measuring code execution
time in C and C++. 

Copyright (C) 2025 A.Prograamar

This library was created for experimental and educational use. 
Please keep expectations reasonable. If you find bugs or have suggestions for 
improvement, contact programmer.amateur@proton.me.

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

#ifndef VI_TIMING_VI_TIMING_C_H
#	define VI_TIMING_VI_TIMING_C_H
#	pragma once

#	define VI_TM_VERSION_MAJOR 0	// 0 - 99
#	define VI_TM_VERSION_MINOR 1	// 0 - 999
#	define VI_TM_VERSION_PATCH 0	// 0 - 9999

#	include <stdint.h>
#	include <stdio.h> // For fputs and stdout

//*******************************************************************************************************************
//
// If VI_TM_STAT_USE_WELFORD defined, uses Welford's method for calculating variance and standard deviation.
// Comment out the next line and rebuild project if you do not need the coefficient of variation and bounce filtering.
#	define VI_TM_STAT_USE_WELFORD
//
// Uses high-performance timing methods (typically platform-specific optimizations like ASM).  
// To switch to standard C11 `timespec_get()` instead, uncomment below and rebuild:  
//#	define VI_TM_USE_STDCLOCK
// 
//*******************************************************************************************************************

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

// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Define: VI_OPTIMIZE_ON/OFF and VI_NOINLINE vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// The VI_OPTIMIZE_ON/OFF macros allow or disable compiler optimizations
// for specific code sections. They must appear outside of a function and takes 
// effect at the first function defined after the macros is seen.
// Usage:
//   VI_OPTIMIZE_OFF
//   /* unoptimized code section */
//   VI_OPTIMIZE_ON
#	if defined(_MSC_VER)
#		define VI_NOINLINE		__declspec(noinline)
#		define VI_OPTIMIZE_OFF	_Pragma("optimize(\"\", off)")
#		define VI_OPTIMIZE_ON	_Pragma("optimize(\"\", on)")
#	elif defined(__clang__)
#		define VI_NOINLINE		[[gnu::noinline]]
#		define VI_OPTIMIZE_OFF	_Pragma("clang optimize push") \
								_Pragma("clang optimize off")
#		define VI_OPTIMIZE_ON	_Pragma("clang optimize pop")
#	elif defined(__GNUC__)
#		define VI_NOINLINE		[[gnu::noinline]]
#		define VI_OPTIMIZE_OFF	_Pragma("GCC push_options") \
								_Pragma("GCC optimize(\"O0\")")
#		define VI_OPTIMIZE_ON	_Pragma("GCC pop_options")
#	else
#		define VI_NOINLINE
#		define VI_OPTIMIZE_OFF
#		define VI_OPTIMIZE_ON
#	endif

// VI_DEBUG_ONLY macro: Expands to its argument only in debug builds, otherwise expands to nothing.
#	ifdef NDEBUG
#		define VI_NDEBUG_ONLY(t) t
#		define VI_DEBUG_ONLY(t)
#	else
#		define VI_NDEBUG_ONLY(t)
#		define VI_DEBUG_ONLY(t) t
#	endif
 
// Stringification and token-pasting macros for unique identifier generation.
#	ifndef __COUNTER__
#		define __COUNTER__ __LINE__ // If __COUNTER__ is not defined, use __LINE__ as a fallback.
#	endif
#	define VI_STR_CONCAT_AUX( a, b ) a##b
#	define VI_STR_CONCAT( a, b ) VI_STR_CONCAT_AUX( a, b )
#	define VI_UNIC_ID( prefix ) VI_STR_CONCAT( prefix, __COUNTER__ )
// Auxiliary macros: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

typedef uint64_t VI_TM_TICK; // Represents a tick count (typically from a high-resolution timer).
typedef uint64_t VI_TM_TDIFF; // Represents a difference between two tick counts (duration).
typedef struct vi_tmJournal_t *VI_TM_HJOUR; // Opaque handle to a timing journal object.
typedef struct vi_tmMeasuring_t *VI_TM_HMEAS; // Opaque handle to a measurement entry within a journal.
typedef int (VI_TM_CALL *vi_tmMeasEnumCallback_t)(VI_TM_HMEAS meas, void* data); // Callback type for enumerating measurements; returning non-zero aborts enumeration.
typedef int (VI_SYS_CALL *vi_tmRptCb_t)(const char* str, void* data); // ABI must be compatible with std::fputs!

typedef struct
{	size_t calls_; // The number of times the measurement was invoked.
	size_t amt_; // The number of all measured elements, including discarded ones.
	VI_TM_TDIFF sum_; // Total time spent measuring all elements, in ticks.
#if defined VI_TM_STAT_USE_WELFORD
	size_t flt_amt_; // Number of items counted. Filtered!
	double flt_mean_; // The mean (average) time taken per processed items. In ticks. Filtered!
	double flt_ss_; // Sum of Squares. In ticks. Filtered!
	size_t flt_calls_; // Number of invokes processed. Filtered!
#endif
} vi_tmMeasuringRAW_t;

typedef enum // Enumeration for various timing information types.
{	VI_TM_INFO_VER,         // unsigned*: Version number of the library.
	VI_TM_INFO_BUILDNUMBER, // unsigned*: Build number of the library.
	VI_TM_INFO_VERSION,     // const char*: Full version string of the library.
	VI_TM_INFO_BUILDTYPE,   // const char*: Build type, either "Release" or "Debug".
	VI_TM_INFO_LIBRARYTYPE, // const char*: Library type, either "Shared" or "Static".
	VI_TM_INFO_RESOLUTION,  // const double*: Clock resolution in ticks.
	VI_TM_INFO_DURATION,    // const double*: Measure duration with cache in seconds.
	VI_TM_INFO_DURATION_EX, // const double*: Measure duration in seconds.
	VI_TM_INFO_OVERHEAD,    // const double*: Clock overhead in ticks.
	VI_TM_INFO_UNIT,        // const double*: Seconds per tick (time unit).

	VI_TM_INFO__COUNT,      // Number of information types.
} vi_tmInfo_e;

typedef enum
{	vi_tmSortByTime = 0x00,
	vi_tmSortByName = 0x01,
	vi_tmSortBySpeed = 0x02,
	vi_tmSortByAmount = 0x03,
	vi_tmSortMask = 0x07,

	vi_tmSortDescending = 0x00,
	vi_tmSortAscending = 0x08,

	vi_tmShowOverhead = 0x0010,
	vi_tmShowUnit = 0x0020,
	vi_tmShowDuration = 0x0040,
	vi_tmShowDurationEx = 0x0080,
	vi_tmShowResolution = 0x0100,
	vi_tmShowMask = 0x1F0,

	vi_tmHideHeader = 0x0200,
	vi_tmDoNotSubtractOverhead = 0x0400, // If set, the overhead is not subtracted from the measured time in report.
} vi_tmReportFlags_e;

#define VI_TM_HGLOBAL ((VI_TM_HJOUR)-1) // Global journal handle, used for global measurements.

#	ifdef __cplusplus
extern "C" {

#	define VI_NODISCARD [[nodiscard]]
#	define VI_NOEXCEPT noexcept
#	define VI_DEF(v) = (v)
#else
#	define VI_NODISCARD
#	define VI_NOEXCEPT
#	define VI_DEF(v)
#endif

// Main functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API VI_NODISCARD VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) VI_NOEXCEPT;

	/// <summary>
	/// Initializes the global journal.
	/// </summary>
	/// <returns>If successful, returns 0.</returns>
	VI_TM_API int VI_TM_CALL vi_tmInit(void);

	/// <summary>
	/// Deinitializes the global journal.
	/// </summary>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmFinit(void);

	/// <summary>
	/// Creates a new journal object and returns a handle to it.
	/// </summary>
	/// <returns>A handle to the newly created journal object, or nullptr if memory allocation fails.</returns>
	VI_TM_API VI_NODISCARD VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate(void);

	/// <summary>
	/// Resets but does not delete all entries in the journal. All entry handles remain valid.
	/// </summary>
	/// <param name="j">The handle to the journal to reset.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmJournalReset(VI_TM_HJOUR j) VI_NOEXCEPT;

	/// <summary>
	/// Closes and deletes a journal handle. All descriptors associated with the journal become invalid.
	/// </summary>
	/// <param name="j">The handle to the journal to be closed and deleted.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOUR j);
	
	/// <summary>
	/// Retrieves a handle to the measurement associated with the given name, creating it if it does not exist.
	/// </summary>
	/// <param name="j">The handle to the journal containing the measurement.</param>
	/// <param name="name">The name of the measurement entry to retrieve.</param>
	/// <returns>A handle to the specified measurement entry within the journal.</returns>
	VI_TM_API VI_NODISCARD VI_TM_HMEAS VI_TM_CALL vi_tmMeasuring(VI_TM_HJOUR j, const char* name);

	/// <summary>
	/// Invokes a callback function for each measurement entry in the journal, allowing early interruption.
	/// </summary>
	/// <param name="j">The handle to the journal containing the measurements.</param>
	/// <param name="fn">A callback function to be called for each measurement. It receives a handle to the measurement and the user-provided data pointer.</param>
	/// <param name="data">A pointer to user-defined data that is passed to the callback function.</param>
	/// <returns>Returns 0 if all measurements were processed. If the callback returns a non-zero value, iteration stops and that value is returned.</returns>
	VI_TM_API int VI_TM_CALL vi_tmMeasuringEnumerate(VI_TM_HJOUR j, vi_tmMeasEnumCallback_t fn, void* data);

	/// <summary>
	/// Performs a measurement replenishment operation by adding a time difference and amount to the measurement object.
	/// </summary>
	/// <param name="m">A handle to the measurement object to be updated.</param>
	/// <param name="tick_diff">The time difference value to add to the measurement.</param>
	/// <param name="amount">The amount associated with the time difference to add.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmMeasuringRepl(VI_TM_HMEAS m, VI_TM_TDIFF duration, size_t amount VI_DEF(1)) VI_NOEXCEPT;

	/// <summary>
	/// Retrieves measurement information from a VI_TM_HMEAS object, including its name, total time, amount, and number of calls.
	/// </summary>
	/// <param name="meas">The measurement handle from which to retrieve information.</param>
	/// <param name="name">Pointer to a string pointer that will receive the name of the measurement. Can be nullptr if not needed.</param>
	/// <param name="total">Pointer to a VI_TM_TDIFF variable that will receive the total measured time. Can be nullptr if not needed.</param>
	/// <param name="amount">Pointer to a size_t variable that will receive the measured amount. Can be nullptr if not needed.</param>
	/// <param name="calls_cnt">Pointer to a size_t variable that will receive the number of calls. Can be nullptr if not needed.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmMeasuringGet(VI_TM_HMEAS m, const char **name, vi_tmMeasuringRAW_t *data);
	
	/// <summary>
	/// Resets the measurement state for the specified measurement handle.
	/// </summary>
	/// <param name="meas">The measurement handle whose state should be reset.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmMeasuringReset(VI_TM_HMEAS m);

	/// <summary>
	/// Retrieves static information about the timing module based on the specified info type.
	/// </summary>
	/// <param name="info">The type of information to retrieve, specified as a value of the vi_tmInfo_e enumeration.</param>
	/// <returns>A pointer to the requested static information. The type of the returned data depends on the info parameter and may point to an unsigned int, a double, or a null-terminated string. Returns nullptr if the info type is not recognized.</returns>
	VI_TM_API VI_NODISCARD const void* VI_TM_CALL vi_tmStaticInfo(vi_tmInfo_e info);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API int VI_SYS_CALL vi_tmRptCb(const char *str, void *data);

	/// <summary>
	/// Generates a report for the specified journal handle, using a callback function to output the report data.
	/// </summary>
	/// <param name="journal_handle">The handle to the journal whose data will be reported.</param>
	/// <param name="flags">Flags that control the formatting and content of the report.</param>
	/// <param name="fn">A callback function used to output each line of the report. If nullptr, defaults to writing to a FILE* stream.</param>
	/// <param name="data">A pointer to user data passed to the callback function. If fn is nullptr and data is nullptr, defaults to stdout.</param>
	/// <returns>The total number of characters written by the report, or a negative value if an error occurs.</returns>
	VI_TM_API int VI_TM_CALL vi_tmReport(VI_TM_HJOUR j, unsigned flags VI_DEF(0), vi_tmRptCb_t VI_DEF(vi_tmRptCb), void* VI_DEF(stdout));

	/// <summary>
	/// Performs a CPU warming routine by running computationally intensive tasks across multiple threads for a specified duration.
	/// </summary>
	/// <param name="threads_qty">The number of threads to use for the warming routine. If zero or greater than the number of available hardware threads, the function uses the maximum available.</param>
	/// <param name="ms">The duration of the warming routine in milliseconds. If zero, the function returns immediately.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmWarming(unsigned threads VI_DEF(0), unsigned ms VI_DEF(1000));

	/// <summary>
	/// Fixates the CPU affinity of the current thread.
	/// </summary>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmCurrentThreadAffinityFixate(void);

	/// <summary>
	/// Restores the CPU affinity of the current thread to its previous state.
	/// </summary>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmCurrentThreadAffinityRestore(void);

	VI_TM_API void VI_TM_CALL vi_tmThreadYield(void);
// Auxiliary functions: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
} // extern "C"
#	endif

#endif // #ifndef VI_TIMING_VI_TIMING_C_H

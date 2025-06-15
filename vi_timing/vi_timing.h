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
#	include <cstring>
#	include <string>
#	include <utility>
#endif

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

// Comment out the next line if you do not need the coefficient of variation and bounce filtering.
#define VI_TM_STAT_USE_WELFORD // If defined, uses Welford's method for calculating variance and standard deviation.

typedef uint64_t VI_TM_TICK; // Represents a tick count (typically from a high-resolution timer).
typedef uint64_t VI_TM_TDIFF; // Represents a difference between two tick counts (duration).
typedef struct vi_tmJournal_t *VI_TM_HJOUR; // Opaque handle to a timing journal object.
typedef struct vi_tmMeasuring_t *VI_TM_HMEAS; // Opaque handle to a measurement entry within a journal.
typedef int (VI_TM_CALL *vi_tmMeasEnumCallback_t)(VI_TM_HMEAS meas, void* data); // Callback type for enumerating measurements; returning non-zero aborts enumeration.

#define VI_TM_HGLOBAL ((VI_TM_HJOUR)-1) // Global journal handle, used for global measurements.

#	ifdef __cplusplus
extern "C" {
#	endif
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
	/// Resets journal items in the storage, either all items or a specific one by name.
	/// </summary>
	/// <param name="journal">The handle to the journal to reset.</param>
	/// <param name="name">The name of the journal item to reset. If null, all items are reset.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmJournalReset(VI_TM_HJOUR j, const char* name VI_DEF(NULL)) VI_NOEXCEPT;

	/// <summary>
	/// Closes and deletes a journal handle.
	/// </summary>
	/// <param name="journal">The handle to the journal to be closed and deleted.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOUR j);
	
	/// <summary>
	/// Retrieves a handle to the measurement associated with the given name, creating it if it does not exist.
	/// </summary>
	/// <param name="journal">The handle to the journal containing the measurement.</param>
	/// <param name="name">The name of the measurement entry to retrieve.</param>
	/// <returns>A handle to the specified measurement entry within the journal.</returns>
	VI_TM_API VI_NODISCARD VI_TM_HMEAS VI_TM_CALL vi_tmMeasuring(VI_TM_HJOUR j, const char* name);

	/// <summary>
	/// Invokes a callback function for each measurement in the journal, allowing early interruption.
	/// </summary>
	/// <param name="journal">The handle to the journal containing the measurements.</param>
	/// <param name="fn">A callback function to be called for each measurement. It receives a handle to the measurement and the user-provided data pointer.</param>
	/// <param name="data">A pointer to user-defined data that is passed to the callback function.</param>
	/// <returns>Returns 0 if all measurements were processed. If the callback returns a non-zero value, iteration stops and that value is returned.</returns>
	VI_TM_API int VI_TM_CALL vi_tmMeasuringEnumerate(VI_TM_HJOUR j, vi_tmMeasEnumCallback_t fn, void* data);

	/// <summary>
	/// Performs a measurement replenishment operation by adding a time difference and amount to the measurement object.
	/// </summary>
	/// <param name="meas">A handle to the measurement object to be updated.</param>
	/// <param name="tick_diff">The time difference value to add to the measurement.</param>
	/// <param name="amount">The amount associated with the time difference to add.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmMeasuringRepl(VI_TM_HMEAS m, VI_TM_TDIFF duration, size_t amount VI_DEF(1)) VI_NOEXCEPT;

	typedef struct vi_tmMeasuringRAW_t
	{
		size_t calls_; // The number of times the measurement was invoked.
		size_t amt_; // The number of all measured elements, including discarded ones.
		VI_TM_TDIFF sum_; // Total time spent measuring all elements, in ticks.
#if defined VI_TM_STAT_USE_WELFORD
		size_t flt_amt_; // Number of items counted. Filtered!
		double flt_mean_; // The mean (average) time taken per processed items. In ticks. Filtered!
		double flt_ss_; // Sum of Squares. In ticks. Filtered!
		size_t flt_calls_; // Number of invoks processed. Filtered!
#endif
	} vi_tmMeasuringRAW_t;

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

	typedef enum // Enumeration for various timing information types.
	{
		VI_TM_INFO_VER,         // unsigned*: Version number of the library.
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

	/// <summary>
	/// Retrieves static information about the timing module based on the specified info type.
	/// </summary>
	/// <param name="info">The type of information to retrieve, specified as a value of the vi_tmInfo_e enumeration.</param>
	/// <returns>A pointer to the requested static information. The type of the returned data depends on the info parameter and may point to an unsigned int, a double, or a null-terminated string. Returns nullptr if the info type is not recognized.</returns>
	VI_TM_API VI_NODISCARD const void* VI_TM_CALL vi_tmStaticInfo(vi_tmInfo_e info);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	typedef int (VI_SYS_CALL *vi_tmRptCb_t)(const char* str, void* data); // ABI must be compatible with std::fputs!
	static inline int VI_SYS_CALL vi_tmRptCb(const char *str, void *data) { return fputs(str, (FILE *)data); }
	typedef enum
	{
		vi_tmSortByTime = 0x00,
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
		vi_tmShowMask = 0xF0,

		vi_tmHideHeader = 0x0100,
		vi_tmDoNotSubtractOverhead = 0x0200, // If set, the overhead is not subtracted from the measured time in report.
	} vi_tmReportFlags_e;

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
	VI_TM_API void VI_TM_CALL vi_tmWarming(unsigned threads VI_DEF(0), unsigned ms VI_DEF(2000));

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

// Define: VI_OPTIMIZE_ON/OFF vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// The following macros allow you to enable or disable compiler optimizations
// for specific code sections. This is useful for benchmarking or debugging
// scenarios where you want to control optimization levels locally.
// Usage:
//   VI_OPTIMIZE_OFF
//   // ... code with optimizations disabled ...
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
// Define: VI_OPTIMIZE_ON/OFF ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 
// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Stringification and token-pasting macros for unique identifier generation.
#	ifndef __COUNTER__
#		define __COUNTER__ __LINE__ // If __COUNTER__ is not defined, use __LINE__ as a fallback.
#	endif
#	define VI_STR_AUX(s) #s
#	define VI_STR(s) VI_STR_AUX(s)
#	define VI_STR_GUM_AUX( a, b ) a##b
#	define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#	define VI_UNIC_ID( prefix ) VI_STR_GUM( prefix, __COUNTER__ )

// VI_DEBUG_ONLY macro: Expands to its argument only in debug builds, otherwise expands to nothing.
#	ifdef NDEBUG
#		define VI_NDEBUG_ONLY(t) t
#		define VI_DEBUG_ONLY(t)
#	else
#		define VI_NDEBUG_ONLY(t)
#		define VI_DEBUG_ONLY(t) t
#	endif
// Auxiliary macros: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
namespace vi_tm
{
	class init_t
	{
		inline static constexpr auto default_callback_fn = &vi_tmRptCb;
		inline static const auto default_callback_data = static_cast<void*>(stdout);

		std::string title_ = "Timing report:\n";
		vi_tmRptCb_t callback_function_ = default_callback_fn;
		void* callback_data_ = default_callback_data;
		unsigned flags_ = vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit | vi_tmShowResolution | vi_tmSortBySpeed;

		init_t(const init_t &) = delete;
		init_t& operator=(const init_t &) = delete;

		void init() const
		{	[[maybe_unused]] const auto result = vi_tmInit();
			assert(0 == result);
		}
		template<typename T, typename... Args>
		void init(T &&v, Args&&... args)
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
	public:
		init_t() { init(); };
		template<typename... Args> explicit init_t(Args&&... args)
			: flags_{0U}
		{	init(std::forward<Args>(args)...);
		}
		~init_t()
		{	if (!title_.empty())
			{	callback_function_(title_.c_str(), callback_data_);
			}
			vi_tmReport(VI_TM_HGLOBAL, flags_, callback_function_, callback_data_);
			vi_tmFinit();
		}
	}; // class init_t

	// measurer_t class: A RAII-style class for measuring code execution time.
	// Unlike the API, this class is not thread-safe!!!
	class measurer_t
	{	VI_TM_HMEAS meas_;
		size_t amt_;
		VI_TM_TICK start_ = 0; // Order matters!!! 'start_' must be initialized last!
	public:
		measurer_t() = delete;
		measurer_t(const measurer_t &) = delete;
		measurer_t(measurer_t &&src) noexcept
		:	meas_{ std::exchange(src.meas_, nullptr) },
			amt_{ std::exchange(src.amt_, 0U) },
			start_{ src.start_ }
		{	assert(meas_);
		}
		measurer_t(VI_TM_HMEAS m, size_t amt = 1) noexcept
		:	meas_{ m },
			amt_{ amt }
		{	assert(meas_);
			if (amt_)
			{	start_ = vi_tmGetTicks();
			}
		}
		~measurer_t() { finish(); }
		void operator=(const measurer_t &) = delete;
		measurer_t &operator=(measurer_t &&src) noexcept
		{	if (this != &src)
			{	meas_ = std::exchange(src.meas_, nullptr);
				amt_ = std::exchange(src.amt_, 0U);
				start_ = src.start_;
			}
			assert(meas_);
			return *this;
		};
		bool is_active() const noexcept
		{	return !!amt_;
		}
		void start(size_t amt = 1) noexcept
		{	assert(!is_active() && amt); // Ensure that the measurer is not already running and that a valid amount is provided.
			amt_ = amt;
			start_ = vi_tmGetTicks(); // Reset start time.
		}
		void stop() noexcept // Stop the measurer without saved time.
		{	amt_ = 0U;
		}
		void finish()
		{	if (is_active())
			{	const auto finish = vi_tmGetTicks();
				vi_tmMeasuringRepl(meas_, finish - start_, amt_);
				amt_ = 0;
			}
		}
	};
} // namespace vi_tm

#		ifndef VI_TM_DISABLE
			// Initializes the global journal and sets up the report callback.
#			define VI_TM_INIT(...) vi_tm::init_t VI_UNIC_ID(_vi_tm_) {__VA_ARGS__}

			// The VI_TM macro creates a measurer_t object with a unique identifier based on the line number.
			// It stores the pointer to the named measurer entry in a static variable. Therefore, it cannot 
			// be called with different measurement names.
#			define VI_TM(...) \
				const auto VI_UNIC_ID(_vi_tm_) = [] (const char* name, size_t amount = 1) -> vi_tm::measurer_t { \
					static const auto meas = vi_tmMeasuring(VI_TM_HGLOBAL, name); /* Static, so as not to waste resources on repeated searches for measurements by name. */ \
					VI_DEBUG_ONLY( \
						const char* registered_name = nullptr; \
						vi_tmMeasuringGet(meas, &registered_name, nullptr); \
						assert(registered_name && 0 == std::strcmp(name, registered_name) && \
							"VI_TM macro used with different names at the same location!"); \
					) \
					return vi_tm::measurer_t{meas, amount}; \
				}(__VA_ARGS__)

			// This macro is used to create a measurer_t object with the function name as the measurement name.
#			define VI_TM_FUNC VI_TM(VI_FUNCNAME)

			// Generates a report for the global journal.
#			define VI_TM_REPORT(...) vi_tmReport(VI_TM_HGLOBAL, __VA_ARGS__)

			// Resets (but not deleted!) the global journal or a specific measurement by name.
			// If you want to reset a specific measurement, pass its name as a string literal, e.g. "my_measurement".
			// If you want to reset all measurements, pass NULL or 0.
#			define VI_TM_RESET(name) vi_tmJournalReset(VI_TM_HGLOBAL, (name)) // If 'name' is zero, then all the meters in the log are reset (but not deleted!).

			// Full version string of the library (Example: 0.1.0.2506151515R static).
#			define VI_TM_FULLVERSION static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_VERSION))
#		else
#			define VI_TM_INIT(...) static const int VI_UNIC_ID(_vi_tm_) = 0
#			define VI_TM(...) static const int VI_UNIC_ID(_vi_tm_) = 0
#			define VI_TM_FUNC ((void)0)
#			define VI_TM_REPORT(...) ((void)0)
#			define VI_TM_RESET ((void)0)
#			define VI_TM_FULLVERSION ""
#		endif
#	endif // #ifdef __cplusplus
#endif // #ifndef VI_TIMING_VI_TIMING_H

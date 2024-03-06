#ifndef VI_TIMING_VI_TIMING_H
#	define VI_TIMING_VI_TIMING_H 0.1
#	pragma once

#if defined(_WIN32)
#	include <Windows.h>
#elif defined(__linux__)
#	include <time.h> // for clock_gettime
#endif

#ifdef _MSC_VER
#	include <intrin.h> // For __rdtscp
#endif

#ifdef __cplusplus
#	include <atomic>
#	include <cstdint>
#	include <cstdio>
#	include <string>
#else
#	include <stdatomic.h>
#	include <stdint.h>
#	include <stdio.h>
#endif

#include <vi/common.h>

#if !defined(__cplusplus) && defined( __STDC_NO_ATOMICS__)
// "<...> we left out support for some C11 optional features such as atomics, <...>"
//	[https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#	error "Atomic objects and the atomic operation library are not supported."
#endif

// Define VI_TM_CALL and VI_TM_API vvvvvvvvvvvvvv
#if defined(_WIN32) // Windows x86 or x64
#	ifdef _WIN64
#		define VI_TM_CALL
#	else
#		define VI_TM_CALL __fastcall
#	endif
#	ifdef vi_timing_EXPORTS
#		define VI_TM_API __declspec(dllexport)
#	else
#		define VI_TM_API __declspec(dllimport)
#	endif
#elif defined(__ANDROID__)
#	define CM_TM_DISABLE "Android not supported"
#elif defined (__linux__)
#	define VI_TM_CALL
#	ifdef vi_timing_EXPORTS
#		define VI_TM_API __attribute__((visibility("default")))
#	else
#		define VI_TM_API
#	endif
#else
#	define CM_TM_DISABLE "Unknown platform"
#endif
// Define VI_TM_CALL and VI_TM_API ^^^^^^^^^^^^^^^^^^^^^^^

typedef VI_STD(uint64_t) vi_tmTicks_t;
typedef int (__cdecl *vi_tmLogRAW)(const char* name, vi_tmTicks_t time, VI_STD(size_t) amount, VI_STD(size_t) calls_cnt, void* data);
typedef int (__cdecl *vi_tmLogSTR)(const char* str, void* data); // Must be compatible with std::fputs!
#ifdef __cplusplus
	using vi_tmAtomicTicks_t = std::atomic<vi_tmTicks_t>;
#else
	typedef _Atomic(vi_tmTicks_t) vi_tmAtomicTicks_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Define vi_tmGetTicks vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#if defined(vi_tmGetTicks)
	/*Custom define*/
#elif defined(_M_X64) || defined(_M_AMD64) // MS compiler on Intel
#   pragma intrinsic(__rdtscp)
	static inline vi_tmTicks_t vi_tmGetTicks_impl(void)
	{
		unsigned int _;
		return __rdtscp(&_);
	}
#elif defined(__x86_64__) || defined(__amd64__) // GNU on Intel
	static inline vi_tmTicks_t vi_tmGetTicks_impl(void) {
		VI_STD(uint32_t) aux;
		VI_STD(uint64_t) low, high;
		__asm__ volatile("rdtscp\n" : "=a" (low), "=d" (high), "=c" (aux));
		return (high << 32) | low;
	}
#elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
	static inline vi_tmTicks_t vi_tmGetTicks_impl(void) {
		VI_STD(uint64_t) result;
		asm volatile("mrs %0, cntvct_el0" : "=r"(result));
		return result;
	}
#elif defined(_WIN32) // Windows
	static inline vi_tmTicks_t vi_tmGetTicks_impl(void)
	{
		LARGE_INTEGER cnt;
		::QueryPerformanceCounter(&cnt);
		return cnt.QuadPart;
	}
#elif defined(__linux__)
	static inline vi_tmTicks_t vi_tmGetTicks_impl(void)
	{
		timespec ts;
		::clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		return 1'000'000'000ULL * ts.tv_sec + ts.tv_nsec;
	}
#else
#	error "You need to define function(s) for your OS and CPU"
#endif

#define vi_tmGetTicks vi_tmGetTicks_impl
// Define vi_tmGetTicks ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	enum vi_tmReportFlags {
		vi_tmSortByTime = 0x00,
		vi_tmSortByName = 0x01,
		vi_tmSortBySpeed = 0x02,
		vi_tmSortByAmount = 0x03,
		vi_tmSortMask = 0x0F,
		vi_tmSortDescending = 0x00,
		vi_tmSortAscending = 0x10,
		vi_tmShowOverhead = 0x20,
		vi_tmShowUnit = 0x40,
		vi_tmShowDuration = 0x80,
	};

	VI_TM_API void VI_TM_CALL vi_tmWarming(int all, VI_STD(size_t) ms); // Superfluous for Intel
	VI_TM_API int VI_TM_CALL vi_tmResults(vi_tmLogRAW fn, void* data);
	VI_TM_API int VI_TM_CALL vi_tmReport(vi_tmLogSTR fn, void* data, VI_STD(uint32_t) flags);

	VI_TM_API vi_tmAtomicTicks_t* VI_TM_CALL vi_tmItem(const char* name, VI_STD(size_t) amount);
	static inline void vi_tmAdd(vi_tmAtomicTicks_t* mem, vi_tmTicks_t start) VI_NOEXCEPT {
		const auto end = vi_tmGetTicks();
		VI_STD(atomic_fetch_add_explicit)(mem, end - start, VI_MEMORY_ORDER(memory_order_relaxed));
	}
#ifdef __cplusplus
} // extern "C" {

namespace vi_tm
{
	inline void warming(bool all = false, std::size_t ms = 256)
	{
		vi_tmWarming(all, ms);
	}

	inline int report(vi_tmLogSTR fn = reinterpret_cast<vi_tmLogSTR>(&std::fputs), void* data = stdout, std::uint32_t flags = 0)
	{
		return vi_tmReport(fn, data, flags);
	}

	class timer_t
	{
		vi_tmAtomicTicks_t* time_;
		const vi_tmTicks_t start_; // Order matters!!! 'start_' must be initialized last!
		timer_t(const timer_t&) = delete;
		timer_t& operator=(const timer_t&) = delete;
	public:
		timer_t(const char* name, std::size_t amount = 1) noexcept : time_{ vi_tmItem(name, amount) }, start_{ vi_tmGetTicks() } {}
		~timer_t() noexcept { vi_tmAdd(time_, start_); }
	};

	class init_t
	{
		std::string title_;
		vi_tmLogSTR cb_;
		void* data_;
		std::uint32_t flags_;
	public:
		init_t(const char* title = "Timing report:", vi_tmLogSTR fn = reinterpret_cast<vi_tmLogSTR>(&std::fputs), void* data = stdout, std::uint32_t flags = 0)
			:title_{ title + std::string{"\n"}}, cb_{fn}, data_{data}, flags_{flags}
		{/**/}

		~init_t()
		{
			if (!title_.empty())
			{
				cb_(title_.c_str(), data_);
			}

			report(cb_, data_, flags_);
		}
	};

#	if defined(VI_TM_DISABLE)
#		define VI_TM_INIT(...) int VI_MAKE_VAR_NAME(_vi_tm_dummy_){(__VA_ARGS__, 0)}
#		define VI_TM(...) int VI_MAKE_VAR_NAME(_vi_tm_dummy_){(__VA_ARGS__, 0)}
#		define VI_TM_REPORT(...) ((void)(__VA_ARGS__, 0))
#	else
#		define VI_TM_INIT(...) vi_tm::init_t VI_MAKE_VAR_NAME(_vi_tm_init_)(__VA_ARGS__)
#		define VI_TM(...) vi_tm::timer_t VI_MAKE_VAR_NAME(_vi_tm_variable_) (__VA_ARGS__)
#		define VI_TM_REPORT(...) vi_tm::report(__VA_ARGS__)
#	endif

#	define VI_TM_FUNC VI_TM( VI_FUNCNAME )
} // namespace vi_tm {
#endif // #ifdef __cplusplus

#endif // #ifndef VI_TIMING_VI_TIMING_H

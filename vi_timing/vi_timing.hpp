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

#ifndef VI_TIMING_VI_TIMING_HPP
#	define VI_TIMING_VI_TIMING_HPP
#	pragma once

#include "vi_timing.h"

#ifdef __cplusplus
#	include <cassert>
#	include <cstring>
#	include <string>
#endif

// Define: VI_OPTIMIZE_ON/OFF vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
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
// Define: VI_OPTIMIZE_ON/OFF ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#	define VI_STR_AUX(s) #s
#	define VI_STR(s) VI_STR_AUX(s)
#	define VI_STR_GUM_AUX( a, b ) a##b
#	define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#	define VI_MAKE_ID( prefix ) VI_STR_GUM( prefix, __LINE__ )
#	ifdef NDEBUG
#		define VI_DEBUG_ONLY(t) /**/
#	else
#		define VI_DEBUG_ONLY(t) t
#	endif
// Auxiliary macros: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
namespace vi_tm
{
	class init_t
	{	inline static constexpr auto default_callback_fn = &vi_tmRptCb;
		inline static const auto default_callback_data = static_cast<void*>(stdout);

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
#			define VI_TM_RESET ((void)0)
#			define VI_TM_FULLVERSION ""
#		else
#			define VI_TM_INIT(...) vi_tm::init_t VI_MAKE_ID(_vi_tm_) {__VA_ARGS__}
			//	The macro VI_TM(const char* name, siz_t amount = 1) stores the pointer to the named measurer
			//	in a static variable to save resources. Therefore, it cannot be used with different names.
#			define VI_TM(...)\
				const auto VI_MAKE_ID(_vi_tm_) = [](const char *name, size_t amount = 1)->vi_tm::measurer_t\
					{	static auto const meas = vi_tmMeasuring(nullptr, name); /*!!! STATIC !!!*/\
						VI_DEBUG_ONLY /* You cannot use the same macro substitution with different names!!! */\
						(	const char* str = nullptr;\
							vi_tmMeasuringGet(meas, &str, nullptr, nullptr, nullptr);\
							assert(0 == std::strcmp(name, str));\
						)\
						return {meas, amount};\
					}(__VA_ARGS__)
#			define VI_TM_FUNC VI_TM( VI_FUNCNAME )
#			define VI_TM_REPORT(...) vi_tmReport(nullptr, __VA_ARGS__)
#			define VI_TM_RESET(name) vi_tmJournalReset(nullptr, (name)) // If 'name' is zero, then all the meters in the log are reset (but not deleted!).
#			define VI_TM_FULLVERSION static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_VERSION))
#		endif
#	endif // #ifdef __cplusplus
#endif // #ifndef VI_TIMING_VI_TIMING_HPP

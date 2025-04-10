// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "misc.h"

#include "build_number_maker.h"
#include "../vi_timing.h"

#ifdef _WIN32
#	include <Windows.h> // SetThreadAffinityMask
#	include <processthreadsapi.h> // GetCurrentProcessorNumber
#elif defined (__linux__)
#	include <pthread.h> // For pthread_setaffinity_np.
#	include <sched.h> // For sched_getcpu.
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace
{
#ifdef NDEBUG
	constexpr char CONFIG[] = "Release";
#else
	constexpr char CONFIG[] = "Debug";
#endif
#ifdef VI_TM_SHARED
	constexpr char TYPE[] = "shared";
#else
	constexpr char TYPE[] = "static";
#endif

	class affinity_fix_t
	{
#if defined(_WIN32)
		DWORD_PTR previous_affinity_{};
#elif defined(__linux__)
		cpu_set_t previous_affinity_{};
#endif
		std::size_t cnt_ = 0U;
		static thread_local affinity_fix_t s_affinity;
		~affinity_fix_t() { assert(0 == cnt_); }
	public:
		static void fixate();
		static void restore();
	};

	thread_local affinity_fix_t affinity_fix_t::s_affinity;

	void affinity_fix_t::fixate()
	{
		if (0 != s_affinity.cnt_++)
		{	return;
		}

#if defined(_WIN32)
		const auto core_id = GetCurrentProcessorNumber();
		const auto affinity = static_cast<DWORD_PTR>(1U) << core_id;
		const auto thread = GetCurrentThread();
		if( (s_affinity.previous_affinity_ = SetThreadAffinityMask(thread, affinity)) != 0)
		{	return; // Ok!
		}
#elif defined(__linux__)
		const auto thread = pthread_self();
		if (0 == pthread_getaffinity_np(thread, sizeof(s_affinity.previous_affinity_), &s_affinity.previous_affinity_))
		{	if (const auto core_id = sched_getcpu(); core_id >= 0)
			{	cpu_set_t affinity;
				CPU_ZERO(&affinity);
				CPU_SET(core_id, &affinity);
				if (0 == pthread_setaffinity_np(thread, sizeof(affinity), &affinity))
				{	return; // Ok!
				}
			}
		}
#endif
		assert(false);
		return; // Fail!
	}

	void affinity_fix_t::restore()
	{	assert(s_affinity.cnt_ > 0);
		if (0 == --s_affinity.cnt_)
		{
#if defined(_WIN32)
			if (s_affinity.previous_affinity_ != 0)
			{	const auto thread = GetCurrentThread();
				[[maybe_unused]] const auto ret = SetThreadAffinityMask(thread, s_affinity.previous_affinity_);
				assert(ret != 0);
			}
#elif defined(__linux__)
			static const cpu_set_t affinity_zero{};
			if (!CPU_EQUAL(&s_affinity.previous_affinity_, &affinity_zero))
			{	const auto thread = pthread_self();
				const auto ret = pthread_setaffinity_np(thread, sizeof(s_affinity.previous_affinity_), &s_affinity.previous_affinity_);
				assert(0 == ret);
			}
#endif
		}
	}
} // namespace

/// <summary>
/// Converts a double to a string representation with specified precision and decimal places.
/// </summary>
/// <param name="val">The value to be converted.</param>
/// <param name="significant">The number of significant digits to round to.</param>
/// <param name="decimal">The number of decimal places to display.</param>
/// <returns>A string representation of the real number.</returns>
/// <remarks>
/// This function handles special cases such as NaN, infinity values.
/// It rounds the number to the specified precision and formats it with the appropriate unit suffix.
/// </remarks>
[[nodiscard]] std::string misc::to_string(double val, unsigned char significant, unsigned char decimal, std::string_view u)
{	assert(decimal < significant);

	std::string result;
	
	if (std::isnan(val))
	{	result = "NaN";
	}
	else if (std::isinf(val))
	{	result = std::signbit(val) ? "-INF" : "INF";
	}
	else
	{	static constexpr auto GROUP_SIZE = 3;

		struct unit_t
		{	double factor_;
			char suffix_[3];
			signed char exp_;
		};
		static constexpr unit_t units[] =
			{	{ 1e-30, " q", -30 }, // quecto
				{ 1e-27, " r", -27 }, // ronto
				{ 1e-24, " y", -24 }, // yocto
				{ 1e-21, " z", -21 }, // zepto
				{ 1e-18, " a", -18 }, // atto
				{ 1e-15, " f", -15 }, // femto
				{ 1e-12, " p", -12 }, // pico
				{ 1e-9, " n", -9 }, // nano
				{ 1e-6, " u", -6 }, // micro
				{ 1e-3, " m", -3 }, // milli
				{ 1.0, "  ", 0 },
				{ 1e3, " k", 3 }, // kilo
				{ 1e6, " M", 6 }, // mega
				{ 1e9, " G", 9 }, // giga
				{ 1e12, " T", 12 }, // tera
				{ 1e15, " P", 15 }, // peta
				{ 1e18, " E", 18 }, // exa
				{ 1e21, " Z", 21 }, // zetta
				{ 1e24, " Y", 24 }, // yotta
				{ 1e27, " R", 27 }, // ronna
				{ 1e30, " Q", 30 }, // quetta
			};
#ifndef NDEBUG
		{	auto pr = [b = std::begin(units)](auto &v)
				{	assert(v.exp_ == b->exp_ + GROUP_SIZE * std::distance(b, &v));
					assert(v.factor_ == std::pow(10, v.exp_));
				};
			std::for_each(std::begin(units), std::end(units), pr);
		}
#endif
		auto unit = units[0];

		if (std::isless(std::abs(val), unit.factor_))
		{	val = 0.0;
		}
		else
		{	{	const auto position_original = static_cast<int>(std::floor(std::log10(std::abs(val))));
				const auto factor = std::pow(10, significant - position_original - 1);
				val = std::round(val * factor) / factor;
			}
			const auto position = static_cast<int>(std::floor(std::log10(std::abs(val))));
			const auto site_position = ((significant - decimal - 1) / GROUP_SIZE) * GROUP_SIZE;
			auto pr = [pullup = position - site_position](auto &v) { return v.exp_ <= pullup; };
			if (auto it = std::find_if(std::rbegin(units), std::rend(units), pr); it != std::rend(units))
			{	unit = *it;
			}
		}

		std::ostringstream ss;
		ss << std::fixed << std::setprecision(decimal) << (val / unit.factor_) << unit.suffix_ << u;
		result = ss.str();
	}

	return result;
}

void VI_TM_CALL vi_tmThreadAffinityFixate()
{	affinity_fix_t::fixate();
}

void VI_TM_CALL vi_tmThreadAffinityRestore()
{	affinity_fix_t::restore();
}

void VI_TM_CALL vi_tmThreadYield(void)
{	std::this_thread::yield();
}

void VI_TM_CALL vi_tmWarming(unsigned int threads_qty, unsigned int m)
{
	if (0 == m)
	{	return;
	}

	if(const auto cores_qty = std::max(1U, std::thread::hardware_concurrency()); 0 == threads_qty || cores_qty < threads_qty)
	{	threads_qty = cores_qty;
	}

	static auto load_dummy = []
		{	volatile auto _ = 0.0;
			for (auto n = 100'000U; n; --n)
			{	_ = _ + std::sin(n) * std::cos(n);
			}
		};

	std::atomic_bool done = false;
	auto thread_func = [&done]
		{	while (!done) //-V776 "Potentially infinite loop."
			{	load_dummy();
			}
		};

	std::vector<std::thread> additional_threads(threads_qty - 1); // Additional threads
	for (auto &t : additional_threads)
	{	t = std::thread{ thread_func };
	}

	for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ m }; ch::steady_clock::now() < stop;)
	{	load_dummy();
	}

	done = true;

	for (auto &t : additional_threads)
	{	if (t.joinable())
		{	t.join();
		}
	}
}

std::uintptr_t VI_TM_CALL vi_tmInfo(vi_tmInfo_e info)
{	std::uintptr_t result = 0U;
	switch (info)
	{
		case VI_TM_INFO_VER:
		{	result = (((VI_TM_VERSION_MAJOR) * 1000U + (VI_TM_VERSION_MINOR)) * 10000U + (VI_TM_VERSION_PATCH));
		} break;

		case VI_TM_INFO_BUILDNUMBER:
		{	result = misc::build_number_get(); //-V101 "Implicit assignment type conversion to memsize type."
		} break;

		case VI_TM_INFO_VERSION:
		{	static const auto version = []
				{	static_assert(VI_TM_VERSION_MAJOR <= 99 && VI_TM_VERSION_MINOR <= 999 && VI_TM_VERSION_PATCH <= 9999); //-V590 "Possible excessive expression or typo."
					std::array<char, std::size("99.999.9999.YYMMDDHHmmC ") - 1 + std::size(TYPE) - 1 + 1> res; //-V1065
					[[maybe_unused]] const auto sz = snprintf
						(	res.data(),
							res.size(),
							VI_STR(VI_TM_VERSION_MAJOR) "." VI_STR(VI_TM_VERSION_MINOR) "." VI_STR(VI_TM_VERSION_PATCH) ".%u%c %s",
							misc::build_number_get(),
							CONFIG[0],
							TYPE
						);
					assert(0 < sz && sz < static_cast<int>(res.size()));
					return res;
				}();
			result = reinterpret_cast<std::uintptr_t>(version.data());
		} break;

		case VI_TM_INFO_BUILDTYPE:
		{	result = reinterpret_cast<std::uintptr_t>(CONFIG);
		} break;

		case VI_TM_INFO_RESOLUTION: // double - Clock resolution [ticks]
		{	static const double resolution = misc::properties_t::props().clock_resolution_;
			result = reinterpret_cast<std::uintptr_t>(&resolution);
		} break;

		case VI_TM_INFO_DURATION: // double - Measure duration [sec]
		{	static const double duration = misc::properties_t::props().all_latency_.count();
			result = reinterpret_cast<std::uintptr_t>(&duration);
		} break;

		case VI_TM_INFO_OVERHEAD: // double - Clock duration [ticks]
		{	static const double overhead = misc::properties_t::props().clock_latency_;
			result = reinterpret_cast<std::uintptr_t>(&overhead);
		} break;

		case VI_TM_INFO_UNIT: // double - Clock duration [sec]
		{	static const double unit = misc::properties_t::props().seconds_per_tick_.count();
			result = reinterpret_cast<std::uintptr_t>(&unit);
		} break;

		default:
		{	assert(false);
		} break;
	}
	return result;
}

#ifndef NDEBUG
namespace
{
	const auto nanotests = []
	{	// nanotest for misc::to_string(double d, unsigned char precision, unsigned char dec)
		const struct
		{	int line_;
			double num_;
			std::string_view expected_;
			unsigned char significant_;
			unsigned char decimal_;
		} tests_set[] =
		{
			{ __LINE__, 0.1, "100 m", 1, 0 },
			{ __LINE__, 1.0, "1  ", 1, 0 },
			{ __LINE__, 2.0, "2  ", 1, 0 },

			{ __LINE__, 0.2111, "200 m", 1, 0 },
			{ __LINE__, 2.111, "2  ", 1, 0 },
			{ __LINE__, 21.11, "20  ", 1, 0 },

			{ __LINE__, 10.0, "10  ", 1, 0 },
			{ __LINE__, 1.0, "1.0  ", 2, 1 },
			{ __LINE__, 1.234, "1  ", 1, 0 },
			{ __LINE__, 1.234, "1.2  ", 2, 1 },

			{ __LINE__, .0, "0 q", 1, 0 },
			{ __LINE__, .0, "0.00 q", 7, 2 },
			{ __LINE__, -9e-31, "0.00 q", 7, 2 },
			{ __LINE__, 9e-31, "0 q", 1, 0 },
			{ __LINE__, -1e-30, "-1 q", 1, 0 },
			{ __LINE__, 1e-30, "1 q", 1, 0 },
			{ __LINE__, 1e-30, "1.00 q", 7, 2 },
			{ __LINE__, -1e33, "-1000 Q", 1, 0 },
			{ __LINE__, 1e33, "1000 Q", 1, 0 },
			{ __LINE__, 1e33, "1000.00 Q", 7, 2 },

			{__LINE__, 0.999, "1.0  ", 2, 1 }, // !!!!

			{ __LINE__, .00555555555,        "6 m", 1, 0 },
			{ __LINE__, .05555555555,       "60 m", 1, 0 },
			{ __LINE__, .55555555555,      "600 m", 1, 0 },
			{ __LINE__, 5.5555555555,        "6  ", 1, 0 },
			{ __LINE__, 55.555555555,       "60  ", 1, 0 },
			{ __LINE__, 555.55555555,      "600  ", 1, 0 },
			{ __LINE__, 5555.5555555,        "6 k", 1, 0 },

			{ __LINE__, .00555555555,        "6 m", 3, 0 },
			{ __LINE__, .05555555555,       "56 m", 3, 0 },
			{ __LINE__, .55555555555,      "556 m", 3, 0 },
			{ __LINE__, 5.5555555555,        "6  ", 3, 0 },
			{ __LINE__, 55.555555555,       "56  ", 3, 0 },
			{ __LINE__, 555.55555555,      "556  ", 3, 0 },
			{ __LINE__, 5555.5555555,        "6 k", 3, 0 },

			{ __LINE__, .00555555555,     "5556 u", 4, 0 },
			{ __LINE__, .05555555555,    "55560 u", 4, 0 },
			{ __LINE__, .55555555555,   "555600 u", 4, 0 },
			{ __LINE__, 5.5555555555,     "5556 m", 4, 0 },
			{ __LINE__, 55.555555555,    "55560 m", 4, 0 },
			{ __LINE__, 555.55555555,   "555600 m", 4, 0 },
			{ __LINE__, 5555.5555555,     "5556  ", 4, 0 },

			{ __LINE__, .00555555555,      "5.6 m", 2, 1 },
			{ __LINE__, .05555555555,     "56.0 m", 2, 1 },
			{ __LINE__, .55555555555,    "560.0 m", 2, 1 },
			{ __LINE__, 5.5555555555,      "5.6  ", 2, 1 },
			{ __LINE__, 55.555555555,     "56.0  ", 2, 1 },
			{ __LINE__, 555.55555555,    "560.0  ", 2, 1 },
			{ __LINE__, 5555.5555555,      "5.6 k", 2, 1 },

			{ __LINE__, .00555555555,      "5.6 m", 4, 1 },
			{ __LINE__, .05555555555,     "55.6 m", 4, 1 },
			{ __LINE__, .55555555555,    "555.6 m", 4, 1 },
			{ __LINE__, 5.5555555555,      "5.6  ", 4, 1 },
			{ __LINE__, 55.555555555,     "55.6  ", 4, 1 },
			{ __LINE__, 555.55555555,    "555.6  ", 4, 1 },
			{ __LINE__, 5555.5555555,      "5.6 k", 4, 1 },

			{ __LINE__, .00555555555,   "5555.6 u", 5, 1 },
			{ __LINE__, .05555555555,  "55556.0 u", 5, 1 },
			{ __LINE__, .55555555555, "555560.0 u", 5, 1 },
			{ __LINE__, 5.5555555555,   "5555.6 m", 5, 1 },
			{ __LINE__, 55.555555555,  "55556.0 m", 5, 1 },
			{ __LINE__, 555.55555555, "555560.0 m", 5, 1 },
			{ __LINE__, 5555.5555555,   "5555.6  ", 5, 1 },

			{ __LINE__, .00555555555,   "5555.56 u", 6, 2 },
			{ __LINE__, .05555555555,  "55555.60 u", 6, 2 },
			{ __LINE__, .55555555555, "555556.00 u", 6, 2 },
			{ __LINE__, 5.5555555555,   "5555.56 m", 6, 2 },
			{ __LINE__, 55.555555555,  "55555.60 m", 6, 2 },
			{ __LINE__, 555.55555555, "555556.00 m", 6, 2 },
			{ __LINE__, 5555.5555555,   "5555.56  ", 6, 2 },
		};

		for (auto &test : tests_set)
		{	const auto reality = misc::to_string(test.num_, test.significant_, test.decimal_, "");
			assert(reality == test.expected_);
		}

		return 0;
	}();
}
#endif // #ifndef NDEBUG

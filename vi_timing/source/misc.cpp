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
#include <cfloat>
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

	constexpr auto GROUP_SIZE = 3;

	struct unit_t
	{	int exp_;
		char suffix_[3];
		friend bool operator <(unit_t l, unit_t r) noexcept { return l.exp_ < r.exp_; }
	} constexpr factors[] =
		{	{ -30, " q" }, // quecto
			{ -27, " r" }, // ronto
			{ -24, " y" }, // yocto
			{ -21, " z" }, // zepto
			{ -18, " a" }, // atto
			{ -15, " f" }, // femto
			{ -12, " p" }, // pico
			{ -9, " n" }, // nano
			{ -6, " u" }, // micro
			{ -3, " m" }, // milli
			{ 0, "  " },
			{ 3, " k" }, // kilo
			{ 6, " M" }, // mega
			{ 9, " G" }, // giga
			{ 12, " T" }, // tera
			{ 15, " P" }, // peta
			{ 18, " E" }, // exa
			{ 21, " Z" }, // zetta
			{ 24, " Y" }, // yotta
			{ 27, " R" }, // ronna
			{ 30, " Q" }, // quetta
		};
		static_assert(0 == factors[0].exp_ % GROUP_SIZE);
		static_assert(GROUP_SIZE * (std::size(factors) - 1) == factors[std::size(factors) - 1].exp_ - factors[0].exp_);

	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr T group(T v) noexcept
	{	if (v < 0)
		{	v -= GROUP_SIZE - 1;
		}
		return v / GROUP_SIZE;
	};
	static_assert(group(9) == 3 && group(2) == 0 && group(0) == 0 && group(-1) == -1 && group(-6) == -2);

	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr T floor_mod(T v) noexcept
	{	auto result = v % GROUP_SIZE;
		if (result < 0)
		{	result += GROUP_SIZE;
		}
		return result;
	};
	static_assert(floor_mod(3) == 0 && floor_mod(2) == 2 && floor_mod(1) == 1 && floor_mod(0) == 0 && floor_mod(-1) == 2 && floor_mod(-2) == 1 && floor_mod(-3) == 0);

	[[nodiscard]] std::string to_string_aux(double val, unsigned char sig, unsigned char const dec)
	{	assert(sig > dec);
		char buff[6] = "??";
		const auto *suffix = buff;

		--sig;
		auto value_v = std::abs(val);

		if (std::isless(value_v, DBL_TRUE_MIN * std::pow(10, sig)))
		{	suffix = factors[0].suffix_; // minimum
			value_v = +0.0;
		}
		else
		{	
			auto value_f = static_cast<int>(std::floor(std::log10(value_v)));
			if (auto d = floor_mod(value_f) - floor_mod(sig - dec); d < 0)
			{	sig += static_cast<unsigned char>(d);
			}

			const auto rounded_f = value_f - sig;
			const auto rounded_v = std::round(value_v * std::pow(10, -rounded_f));
			if (auto f = static_cast<int>(std::floor(std::log10(rounded_v))); f != sig)
			{	assert(f == sig + 1);
				++value_f;
			}

			const auto group_pos = (group(value_f) - group(sig - dec)) * GROUP_SIZE;
			if (const auto idx = (group_pos - factors[0].exp_) / GROUP_SIZE; idx >= 0 && idx < std::size(factors))
			{	assert(factors[idx].exp_ == group_pos);
				suffix = factors[idx].suffix_;
			}
			else
			{	std::snprintf(buff, std::size(buff), "e%d", group_pos);
			}

			value_v = rounded_v * std::pow(10, rounded_f - GROUP_SIZE * (group(value_f) - group(sig - dec)));
			value_v = std::copysign(value_v, val);
		}

		assert(0 == errno);

		std::stringstream ss;
		ss << std::fixed << std::setprecision(dec) << value_v << suffix;
		return ss.str();
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
/// It rounds the number to the specified precision and formats it with the appropriate suffix.
/// </remarks>
[[nodiscard]] std::string misc::to_string(double val, unsigned char significant, unsigned char decimal)
{	assert(significant > decimal);
	if (decimal >= significant)
	{	return "ERR";
	}
	if (std::isnan(val))
	{	return "NaN";
	}
	if (std::isinf(val))
	{	return std::signbit(val) ? "-INF" : "INF";
	}
	return to_string_aux(val, significant, decimal);
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
	const auto nanotest_factors = []
	{	auto pr = [](auto &v) { assert(v.exp_ - factors[0].exp_ == GROUP_SIZE * std::distance(factors, &v)); };
		std::for_each(factors, factors + std::size(factors), pr);
		return 0;
	}();

	const auto nanotest_to_string = []
	{	assert(0 == errno);

		// nanotest for misc::to_string(double d, unsigned char precision, unsigned char dec)
		const struct
		{	int line_; //-V802
			double num_;
			std::string_view expected_;
			unsigned char significant_;
			unsigned char decimal_;
		} tests_set[] =
		{
			{ __LINE__, .0, "0 q", 1, 0 },
			{ __LINE__, -0.0009123e-30, "-912300.00e-39", 7, 2 },
//			{__LINE__, DBL_MIN * 10, "0.0 q", 3, 1},
//****************
			//{__LINE__, DBL_MIN, "0.0 q", 5, 1}, // 2.2250738585072014e-308
			//{__LINE__, DBL_MIN * 10, "0.0 q", 3, 1},
			//{__LINE__, DBL_MIN * 100, "2.2e-306", 3, 1}, // 2.2250738585072014e-306
			//{__LINE__, DBL_MIN * 100, "0.0 q", 4, 1}, // 2225.0738585072014e-309
			//{__LINE__, DBL_MIN * 1000, "22.3e-306", 4, 1}, // 22.2500738585072014e-306
			//{__LINE__, DBL_MIN / 10, "0 q", 1, 0}, // 2.2250738585072014e-309
			//{__LINE__, DBL_MIN / 10, "0.0 q", 5, 1}, // 2.2250738585072014e-309
			//{__LINE__, DBL_MIN / 100, "0 q", 1, 0}, // 2.2250738585072014e-310

			{__LINE__, 1e30, "1 Q", 1, 0},
			{__LINE__, 900e30, "900 Q", 1, 0},
			{__LINE__, 900e30, "900.0 Q", 2, 1},
			{__LINE__, 999e30, "1.0e33", 2, 1},
			{__LINE__, 999.9e30, "999.9 Q", 4, 1},
			{__LINE__, 999.9e30, "999900.0 R", 5, 1},
			{__LINE__, 999.99e30, "999990.0 R", 5, 1},

			{__LINE__, DBL_MAX, "180.0e306", 3, 1}, // 1.7976931348623158e+308
			{__LINE__, DBL_MAX, "179770.0e303", 5, 1}, // 179769.31348623158e+303

			{__LINE__, -0.0123, "-12.0 m", 2, 1},
			{__LINE__, -0.0123, "-12300.0 u", 5, 1},

			{ __LINE__, 123.456789, "120.0  ", 2, 1 },
			{ __LINE__, 1.23456789, "1234.6 m", 5, 1 },
			{ __LINE__, -0.00123456, "-1234.6 u", 5, 1 },
			{ __LINE__, -0.00999999, "-10000.0 u", 5, 1 },

			{ __LINE__, 1e-30, "1 q", 1, 0 },
			{ __LINE__, 1e-30, "1.0 q", 2, 1 },
			{ __LINE__, 0.51e-30, "500e-33", 1, 0 },
			{ __LINE__, 0.49e-30, "500e-33", 1, 0 },
			{ __LINE__, 0.91e-30, "910.0e-33", 2, 1 },

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
			{ __LINE__, -0.0009123e-30, "-912300.00e-39", 7, 2 },
			{ __LINE__, -0.009123e-30, "-9123.00e-36", 7, 2 },
			{ __LINE__, -0.09123e-30, "-91230.00e-36", 7, 2 },
			{ __LINE__, -0.9123e-30, "-912300.00e-36", 7, 2 },
			{ __LINE__, -91.2345e30, "-91234.50 R", 7, 2 },
			{ __LINE__, -91'234.5e30, "-91234.50 Q", 7, 2 },
			{ __LINE__, 9e-31, "900e-33", 1, 0 },
			{ __LINE__, -1e-30, "-1 q", 1, 0 },
			{ __LINE__, 1e-30, "1 q", 1, 0 },
			{ __LINE__, 1e-30, "1000.00e-33", 7, 2 },
			{ __LINE__, -1e33, "-1e33", 1, 0 },
			{ __LINE__, 1e33, "1e33", 1, 0 },
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
		{	const auto reality = misc::to_string(test.num_, test.significant_, test.decimal_);
			assert(reality == test.expected_);
		}
		
		assert(0 == errno);

		return 0;
	}();
}
#endif // #ifndef NDEBUG

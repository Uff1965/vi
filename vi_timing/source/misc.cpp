// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "misc.h"

#include "build_number_maker.h"
#include "../vi_timing_c.h"

#ifdef _WIN32
#	include <Windows.h> // SetThreadAffinityMask
#	include <processthreadsapi.h> // GetCurrentProcessorNumber
#elif defined (__linux__)
#	include <pthread.h> // For pthread_setaffinity_np.
#	include <sched.h> // For sched_getcpu.
#endif

#include <atomic> // for atomic_bool
#include <array>
#include <cassert>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

//-V::-V2600

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

	namespace affinity
	{
#if defined(_WIN32)
		using thread_affinity_mask_t = DWORD_PTR;

		std::optional<thread_affinity_mask_t> set_affinity()
		{	const auto affinity = static_cast<thread_affinity_mask_t>(1U) << GetCurrentProcessorNumber();
			if (const auto ret = SetThreadAffinityMask(GetCurrentThread(), affinity))
				return ret;
			return {};
		}

		bool restore_affinity(thread_affinity_mask_t prev)
		{	if (0 != prev)
			{	if (misc::verify(0 != SetThreadAffinityMask(GetCurrentThread(), prev)))
				{	return true;
				}
			}
			return false;
		}
#elif defined(__linux__)
		using thread_affinity_mask_t = cpu_set_t;

		std::optional<thread_affinity_mask_t> set_affinity()
		{	const auto thread = pthread_self();
			if (thread_affinity_mask_t prev{}; verify(0 == pthread_getaffinity_np(thread, sizeof(prev), &prev)))
			{	if (const auto core_id = sched_getcpu(); verify(core_id >= 0))
				{	cpu_set_t affinity;
					CPU_ZERO(&affinity);
					CPU_SET(core_id, &affinity);
					if (verify(0 == pthread_setaffinity_np(thread, sizeof(affinity), &affinity)))
					{	return prev; // Ok!
					}
				}
			}
			return {};
		}

		bool restore_affinity(thread_affinity_mask_t prev)
		{	static const cpu_set_t affinity_zero{};
			if (!CPU_EQUAL(&prev, &affinity_zero))
			{	if (verify(0 == pthread_setaffinity_np(pthread_self(), sizeof(prev), &prev)))
				{	return true;
				}
			}
			return false;
		}
#endif

		class affinity_fix_t
		{	static thread_local affinity_fix_t s_instance; // Thread-local instance!!!
			std::size_t cnt_ = 0U;
			thread_affinity_mask_t previous_affinity_{};

			~affinity_fix_t() { assert(0 == cnt_); }
		public:
			static int fixate()
			{	if (0 == s_instance.cnt_++)
				{	auto prev = set_affinity();
					if (!misc::verify(!!prev))
					{	return VI_EXIT_FAILURE();
					}
					assert(!!prev);
					s_instance.previous_affinity_ = *prev;
				}
				return VI_EXIT_SUCCESS();
			}
			static int restore()
			{	assert(s_instance.cnt_ > 0);
				if (0 == --s_instance.cnt_)
				{	if(!misc::verify(restore_affinity(s_instance.previous_affinity_)))
					{	return VI_EXIT_FAILURE();
					}
					s_instance.previous_affinity_ = thread_affinity_mask_t{};
				}
				return VI_EXIT_SUCCESS();
			}
		};
		thread_local affinity_fix_t affinity_fix_t::s_instance;
	} // namespace affinity

	namespace to_str
	{
		constexpr auto GROUP_SIZE = 3;

		struct
		{	int exp_;
			char suffix_[3];
		} constexpr factors[]
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
		static_assert(GROUP_SIZE *(std::size(factors) - 1) == factors[std::size(factors) - 1].exp_ - factors[0].exp_);

		const char *get_suffix(int group_pos, std::array<char, 6> &buff)
		{	if (const auto idx = (group_pos - factors[0].exp_) / GROUP_SIZE; idx >= 0 && idx < std::size(factors))
			{	assert(factors[idx].exp_ == group_pos);
				return factors[idx].suffix_;
			}
			std::snprintf(buff.data(), buff.size(), "e%d", group_pos);
			return buff.data();
		}

		template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
		constexpr T group(T v) noexcept
		{	return (v < 0 ? v - GROUP_SIZE + 1 : v) / GROUP_SIZE;
		};
		static_assert(group(9) == 3 && group(2) == 0 && group(0) == 0 && group(-1) == -1 && group(-6) == -2);

		template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
		constexpr T floor_mod(T v) noexcept
		{	v %= GROUP_SIZE;
			return v < 0 ? v + GROUP_SIZE : v;
		}
		static_assert(floor_mod(3) == 0 && floor_mod(2) == 2 && floor_mod(1) == 1 && floor_mod(0) == 0 && floor_mod(-1) == 2 && floor_mod(-2) == 1 && floor_mod(-3) == 0);

		std::tuple<double, const char *> to_string_aux2(double val_org, int sig_pos, unsigned char const dec, std::array<char, 6> &buff)
		{	assert(std::isgreaterequal(std::abs(val_org), DBL_MIN) && sig_pos >= dec);

			auto val = std::abs(val_org);
			auto fact = static_cast<int>(std::floor(std::log10(val)));
			if (auto d = floor_mod(sig_pos - dec) - floor_mod(fact); d > 0)
			{	sig_pos -= d;
			}

			const auto rounded_f = fact - sig_pos;
			{	auto exp = -rounded_f;
				while (exp >= DBL_MAX_10_EXP)
				{	static const auto MAX10 = std::pow(10, DBL_MAX_10_EXP);
					val *= MAX10;
					exp -= DBL_MAX_10_EXP;
				}
				val *= std::pow(10, exp);
			}

			val = std::round(val);
			if (auto fact_rounded = static_cast<int>(std::floor(std::log10(val))); fact_rounded != sig_pos)
			{	assert(fact_rounded == sig_pos + 1);
				++fact;
			}

			const auto group_pos = (group(fact) - group(sig_pos - dec)) * GROUP_SIZE;
			val *= std::pow(10, rounded_f - group_pos);
			assert(0 == errno);
			return { std::copysign(val, val_org), get_suffix(group_pos, buff) };
		}

		std::string to_string_aux(double val_org, unsigned char sig, unsigned char const dec)
		{	assert(sig > dec && 0 == errno);
			std::array<char, 6> buff;

			const auto [val, suffix] = std::isnormal(val_org) ?
				to_string_aux2(val_org, sig - 1, dec, buff) :
				std::make_tuple(+0.0, "  ");

			std::string result(sig + (9 + 1), '\0'); // 2.1 -> "-  2.2e-308" -> 9 + 2; 6.2 -> "-  6666.66e-308" -> 9 + 6;
			const auto len = static_cast<std::size_t>(std::snprintf(result.data(), result.size(), "%.*f%s", dec, val, suffix));
			if (misc::verify(result.size() > len)) //-V201 // Error will be converted too long unsigned integer.
			{	result.resize(len);
			}
			else
			{	result = "ERR";
			}
			return result;
		}
	} // namespace to_str

	void busy()
	{	volatile auto f = 0.0;
		for (auto n = 10'000U; n; --n)
		{	// To keep the CPU busy.
			f = (f + std::sin(n) * std::cos(n)) / 1.0001;
			std::atomic_thread_fence(std::memory_order_relaxed);
		}
	};

} // namespace

[[nodiscard]] std::string misc::to_string(double val, unsigned char significant, unsigned char decimal)
{	if (!verify(decimal < significant))
	{	return "ERR";
	}
	if (std::isnan(val))
	{	return "NaN";
	}
	if (std::isinf(val))
	{	return std::signbit(val) ? "-INF" : "INF";
	}
	return to_str::to_string_aux(val, significant, decimal);
}

int VI_TM_CALL vi_CurrentThreadAffinityFixate()
{	return affinity::affinity_fix_t::fixate();
}

int VI_TM_CALL vi_CurrentThreadAffinityRestore()
{	return affinity::affinity_fix_t::restore();
}

void VI_TM_CALL vi_ThreadYield(void) noexcept
{	std::this_thread::yield();
}

int  VI_TM_CALL vi_Warming(unsigned int threads_qty, unsigned int ms)
{	if (0 == ms)
	{	return VI_EXIT_SUCCESS();
	}

	if (threads_qty == 0U || threads_qty > std::thread::hardware_concurrency())
	{	threads_qty = std::thread::hardware_concurrency();
	}
	if (threads_qty)
	{	threads_qty--;
	}

	try
	{	std::atomic_bool done = false;
		std::vector<std::thread> additional_threads;
		additional_threads.reserve(static_cast<std::size_t>(threads_qty)); //-V201
		for (unsigned i = 0; i < threads_qty; ++i)
		{	additional_threads.emplace_back([&done] { while (!done) busy(); });
		}

		for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ ms }; ch::steady_clock::now() < stop;)
		{	busy();
		}

		done = true;
		for (auto &t : additional_threads)
		{	if (t.joinable())
			{	t.join();
			}
		}
	}
	catch (...)
	{	assert(false);
		return VI_EXIT_FAILURE();
	}

	return VI_EXIT_SUCCESS();
}

const void* VI_TM_CALL vi_tmStaticInfo(vi_tmInfo_e info)
{	switch (info)
	{
		case VI_TM_INFO_VER:
		{	static const unsigned ver = (VI_TM_VERSION_MAJOR * 1000U + VI_TM_VERSION_MINOR) * 10000U + VI_TM_VERSION_PATCH;
			return &ver;
		}

		case VI_TM_INFO_BUILDNUMBER:
		{	static const unsigned build = misc::build_number_get();
			return &build;
		}

		case VI_TM_INFO_VERSION:
		{	static const auto version = []
				{	static_assert(VI_TM_VERSION_MAJOR <= 99 && VI_TM_VERSION_MINOR <= 999 && VI_TM_VERSION_PATCH <= 9999);
					std::array<char, (std::size("99.999.9999.YYMMDDHHmm") - 1) + 2 + (std::size(TYPE) - 1) + 1> result; //-V1065
					[[maybe_unused]] const auto sz = snprintf
					(	result.data(),
						result.size(),
						"%u.%u.%u.%u%c %s",
						VI_TM_VERSION_MAJOR,
						VI_TM_VERSION_MINOR,
						VI_TM_VERSION_PATCH,
						misc::build_number_get(),
						CONFIG[0],
						TYPE
					);
					assert(0 < sz && static_cast<std::size_t>(sz) < result.size()); //-V201
					return result;
				}();
			return version.data();
		}

		case VI_TM_INFO_BUILDTYPE:
			return CONFIG;

		case VI_TM_INFO_LIBRARYTYPE:
			return TYPE;

		case VI_TM_INFO_RESOLUTION:
		{	static const double resolution = misc::properties_t::props().clock_resolution_ticks_;
			return &resolution;
		}

		case VI_TM_INFO_DURATION:
		{	static const double duration = misc::properties_t::props().duration_threadsafe_.count();
			return &duration;
		}

		case VI_TM_INFO_DURATION_UNSAFE:
		{	static const double duration = misc::properties_t::props().duration_non_threadsafe_.count();
			return &duration;
		}

		case VI_TM_INFO_DURATION_EX:
		{	static const double duration_ex = misc::properties_t::props().duration_ex_threadsafe_.count();
			return &duration_ex;
		}

		case VI_TM_INFO_OVERHEAD:
		{	static const double overhead = misc::properties_t::props().clock_latency_ticks_;
			return &overhead;
		}

		case VI_TM_INFO_UNIT:
		{	static const double unit = misc::properties_t::props().seconds_per_tick_.count();
			return &unit;
		}

		default:
			static_assert(VI_TM_INFO_COUNT_ == 11, "Not all vi_tmInfo_e enum values are processed in the function vi_tmStaticInfo.");
			assert(false); // If we reach this point, the info type is not recognized.
			return nullptr;
	}
} // vi_tmStaticInfo(vi_tmInfo_e info)

#ifndef NDEBUG
namespace
{	// nanotest to array consistency to_str::factors.
	const auto nanotest_factors = []
	{	for (auto &v: to_str::factors)
		{	assert(v.exp_ == to_str::factors[0].exp_ + to_str::GROUP_SIZE * std::distance(to_str::factors, &v));
		}
		return 0;
	}();

	const auto nanotest_to_string = []
	{	// nanotest for misc::to_string(double d, unsigned char precision, unsigned char dec)
		struct
		{	int line_; double value_; std::string_view expected_; unsigned char significant_; unsigned char decimal_; // -V802
		} static const tests_set[] =
		{
//****************
			// The following test cases check the boundary and special floating-point values for the misc::to_string function.
			// They ensure correct handling of INF, -INF, NaN, -NaN, DBL_MAX, DBL_MIN, DBL_TRUE_MIN, and values near zero.
			{__LINE__, 0.0, "0.0  ", 2, 1},
			{__LINE__, NAN, "NaN", 2, 1},									{__LINE__, -NAN, "NaN", 2, 1},
			{__LINE__, std::nextafter( 0.0, 1.0), "0.0  ", 2, 1},			{__LINE__, std::nextafter(-0.0, -1.0), "0.0  ", 2, 1},
			{__LINE__, DBL_TRUE_MIN, "0.0  ", 2, 1},						{__LINE__, -DBL_TRUE_MIN, "0.0  ", 2, 1},
			{__LINE__, std::nextafter( DBL_TRUE_MIN, 1.0), "0.0  ", 2, 1},	{__LINE__, std::nextafter(-DBL_TRUE_MIN, -1.0), "0.0  ", 2, 1},
			{__LINE__, std::nextafter( DBL_MIN, 0.0), "0.0  ", 2, 1},		{__LINE__, std::nextafter(-DBL_MIN, 0.0), "0.0  ", 2, 1},
			{__LINE__, DBL_MIN, "22.0e-309", 2, 1},							{__LINE__, -DBL_MIN, "-22.0e-309", 2, 1},
			{__LINE__, 3.14159, "3.1  ", 2, 1},								{__LINE__, -3.14159, "-3.1  ", 2, 1},
			{__LINE__, DBL_MAX, "180.0e306", 2, 1},							{__LINE__, -DBL_MAX, "-180.0e306", 2, 1},
			{__LINE__, DBL_MAX * (1.0 + DBL_EPSILON),  "INF", 2, 1},		{__LINE__, -DBL_MAX * (1.0 + DBL_EPSILON), "-INF", 2, 1},
			// Test cases for SI prefixes and scientific notation formatting in misc::to_string.
			// Each entry checks that a value like 1e3, 1e-3, etc., is formatted with the correct SI suffix or scientific notation.
			{ __LINE__, 1e-306,"1.0e-306", 2, 1 },	{ __LINE__, -1e-306,"-1.0e-306", 2, 1 },
			{ __LINE__, 1e-30,"1.0 q", 2, 1 },		{ __LINE__, -1e-30,"-1.0 q", 2, 1 },
			{ __LINE__, 1e-27,"1.0 r", 2, 1 },		{ __LINE__, -1e-27,"-1.0 r", 2, 1 },
			{ __LINE__, 1e-24,"1.0 y", 2, 1 },		{ __LINE__, -1e-24,"-1.0 y", 2, 1 },
			{ __LINE__, 1e-21,"1.0 z", 2, 1 },		{ __LINE__, -1e-21,"-1.0 z", 2, 1 },
			{ __LINE__, 1e-18,"1.0 a", 2, 1 },		{ __LINE__, -1e-18,"-1.0 a", 2, 1 },
			{ __LINE__, 1e-15,"1.0 f", 2, 1 },		{ __LINE__, -1e-15,"-1.0 f", 2, 1 },
			{ __LINE__, 1e-12,"1.0 p", 2, 1 },		{ __LINE__, -1e-12,"-1.0 p", 2, 1 },
			{ __LINE__, 1e-9,"1.0 n", 2, 1 },		{ __LINE__, -1e-9,"-1.0 n", 2, 1 },
			{ __LINE__, 1e-6,"1.0 u", 2, 1 },		{ __LINE__, -1e-6,"-1.0 u", 2, 1 },
            { __LINE__, 1e-3,"1.0 m", 2, 1 },		{ __LINE__, -1e-3,"-1.0 m", 2, 1 },
            { __LINE__, 1e0,"1.0  ", 2, 1 },		{ __LINE__, -1e0,"-1.0  ", 2, 1 },
            { __LINE__, 1e3,"1.0 k", 2, 1 },		{ __LINE__, -1e3,"-1.0 k", 2, 1 },
            { __LINE__, 1e6,"1.0 M", 2, 1 },		{ __LINE__, -1e6,"-1.0 M", 2, 1 },
            { __LINE__, 1e9,"1.0 G", 2, 1 },		{ __LINE__, -1e9,"-1.0 G", 2, 1 },
            { __LINE__, 1e12,"1.0 T", 2, 1 },		{ __LINE__, -1e12,"-1.0 T", 2, 1 },
            { __LINE__, 1e15,"1.0 P", 2, 1 },		{ __LINE__, -1e15,"-1.0 P", 2, 1 },
            { __LINE__, 1e18,"1.0 E", 2, 1 },		{ __LINE__, -1e18,"-1.0 E", 2, 1 },
            { __LINE__, 1e21,"1.0 Z", 2, 1 },		{ __LINE__, -1e21,"-1.0 Z", 2, 1 },
            { __LINE__, 1e24,"1.0 Y", 2, 1 },		{ __LINE__, -1e24,"-1.0 Y", 2, 1 },
            { __LINE__, 1e27,"1.0 R", 2, 1 },		{ __LINE__, -1e27,"-1.0 R", 2, 1 },
            { __LINE__, 1e30,"1.0 Q", 2, 1 },		{ __LINE__, -1e30,"-1.0 Q", 2, 1 },
            { __LINE__, 1e306,"1.0e306", 2, 1 },	{ __LINE__, -1e306,"-1.0e306", 2, 1 },
			// rounding tests.
			{__LINE__, 1.19, "1.2  ", 2, 1},		{__LINE__, -1.19, "-1.2  ", 2, 1}, // simple
			{__LINE__, 9.99, "10.0  ", 2, 1},		{__LINE__, -9.99, "-10.0  ", 2, 1},
			{ __LINE__, 1.349, "1.3  ", 2, 1 },		{ __LINE__, -1.349, "-1.3  ", 2, 1 },
			{ __LINE__, 1.35, "1.4  ", 2, 1 },		{ __LINE__, -1.35, "-1.4  ", 2, 1 }, // 0.5 rounding up.
			// test group sellect
			{__LINE__, 0.0001, "100.0 u", 2, 1},	{__LINE__, -0.0001, "-100.0 u", 2, 1},
			{__LINE__, 0.001, "1.0 m", 2, 1},		{__LINE__, -0.001, "-1.0 m", 2, 1},
			{__LINE__, 0.01, "10.0 m", 2, 1},		{__LINE__, -0.01, "-10.0 m", 2, 1},
			{__LINE__, 0.1, "100.0 m", 2, 1},		{__LINE__, -0.1, "-100.0 m", 2, 1},
			{__LINE__, 1.0, "1.0  ", 2, 1},			{__LINE__, -1.0, "-1.0  ", 2, 1},
			{__LINE__, 10.0, "10.0  ", 2, 1},		{__LINE__, -10.0, "-10.0  ", 2, 1},
			{__LINE__, 100.0, "100.0  ", 2, 1},		{__LINE__, -100.0, "-100.0  ", 2, 1},
			{__LINE__, 1000.0, "1.0 k", 2, 1},		{__LINE__, -1000.0, "-1.0 k", 2, 1},
			{__LINE__, 0.1, "100000.0 u", 5, 1},	{__LINE__, -0.1, "-100000.0 u", 5, 1},
			{__LINE__, 1.0, "1000.0 m", 5, 1},		{__LINE__, -1.0, "-1000.0 m", 5, 1},
			{__LINE__, 10.0, "10000.0 m", 5, 1},	{__LINE__, -10.0, "-10000.0 m", 5, 1},
			{__LINE__, 100.0, "100000.0 m", 5, 1},	{__LINE__, -100.0, "-100000.0 m", 5, 1},
			{__LINE__, 1000.0, "1000.0  ", 5, 1},	{__LINE__, -1000.0, "-1000.0  ", 5, 1},
		};
		errno = 0;

		for (auto &test : tests_set)
		{	const auto reality = misc::to_string(test.value_, test.significant_, test.decimal_);
			assert(reality == test.expected_);
		}
		assert(0 == errno);
		return 0;
	}();
}
#endif // #ifndef NDEBUG

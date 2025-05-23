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

#include <array>
#include <atomic>
#include <cassert>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <optional>
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

	inline bool verify(bool b) { assert(b); return b; }

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
			{	if (verify(0 != SetThreadAffinityMask(GetCurrentThread(), prev)))
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
		{	static thread_local affinity_fix_t s_instance;
			std::size_t cnt_ = 0U;
			thread_affinity_mask_t previous_affinity_{};

			~affinity_fix_t() { assert(0 == cnt_); }
		public:
			static void fixate()
			{	if (0 == s_instance.cnt_++)
				{	if (auto prev = set_affinity(); verify(!!prev))
					{	s_instance.previous_affinity_ = *prev;
					}
				}
				return;
			}
			static void restore()
			{	assert(s_instance.cnt_ > 0);
				if (0 == --s_instance.cnt_ && restore_affinity(s_instance.previous_affinity_))
				{	s_instance.previous_affinity_ = thread_affinity_mask_t{};
				}
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
		{ { -30, " q" }, // quecto
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
			if (auto len = std::snprintf(result.data(), result.size(), "%.*f%s", dec, val, suffix); verify(len >= 0))
			{	assert(result.size() > len);
				result.resize(len);
			}
			else
			{	result = "ERR";
			}
			return result;
		}
	} // namespace to_str
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

void VI_TM_CALL vi_tmCurrentThreadAffinityFixate()
{	affinity::affinity_fix_t::fixate();
}

void VI_TM_CALL vi_tmCurrentThreadAffinityRestore()
{	affinity::affinity_fix_t::restore();
}

void VI_TM_CALL vi_tmThreadYield(void)
{	std::this_thread::yield();
}

void VI_TM_CALL vi_tmWarming(unsigned int threads_qty, unsigned int ms)
{	if (0 == ms)
	{	return;
	}

	if(const auto cores_qty = std::max(1U, std::thread::hardware_concurrency()); 0 == threads_qty || cores_qty < threads_qty)
	{	threads_qty = cores_qty;
	}

	std::atomic_bool done = false;
	static auto load_dummy = [] { volatile auto f = 0.0; for (auto n = 10'000U; n; --n) f += std::sin(n) * std::cos(n); };
	auto thread_func = [&done] { while (!done) load_dummy(); };

	std::vector<std::thread> additional_threads(threads_qty - 1); // Additional threads
	for (auto &t : additional_threads)
	{	t = std::thread{ thread_func };
	}
	for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ ms }; ch::steady_clock::now() < stop;)
	{	load_dummy();
	}

	done = true;

	for (auto &t : additional_threads)
	{	t.join();
	}
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
					std::array<char, (std::size("99.999.9999.YYMMDDHHmm") - 1) + 2 + (std::size(TYPE) - 1) + 1> result;
					[[maybe_unused]] const auto sz = snprintf
					(result.data(),
						result.size(),
						VI_STR(VI_TM_VERSION_MAJOR) "." VI_STR(VI_TM_VERSION_MINOR) "." VI_STR(VI_TM_VERSION_PATCH) "." "%u%c %s",
						misc::build_number_get(),
						CONFIG[0],
						TYPE
					);
					assert(0 < sz && sz < static_cast<int>(result.size()));
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
		{	static const double duration = misc::properties_t::props().all_latency_.count();
			return &duration;
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
			static_assert(VI_TM_INFO__COUNT == 9, "Not all vi_tmInfo_e enum values are processed in the function vi_tmStaticInfo.");
			assert(false); // If we reach this point, the info type is not recognized.
			return nullptr;
	}
} // vi_tmStaticInfo(vi_tmInfo_e info)

#ifndef NDEBUG
namespace
{
	const auto nanotest_factors = []
	{	for (auto &v: to_str::factors)
		{	assert(v.exp_ == to_str::factors[0].exp_ + to_str::GROUP_SIZE * std::distance(to_str::factors, &v));
		}
		return 0;
	}();

	const auto nanotest_to_string = []
	{	// nanotest for misc::to_string(double d, unsigned char precision, unsigned char dec)
		struct
		{	int line_;
			double value_;
			std::string_view expected_;
			unsigned char significant_;
			unsigned char decimal_;
		} static const tests_set[] =
		{
//****************
			{__LINE__, NAN, "NaN", 1, 0},
			{__LINE__, -NAN, "NaN", 1, 0},
			{__LINE__, DBL_MAX * 1.1, "INF", 7, 2},
			{__LINE__, DBL_MAX + 1e300, "INF", 3, 1},
			{__LINE__, -DBL_MAX - 1e300, "-INF", 1, 0},
			{__LINE__, DBL_MAX, "180.0e306", 3, 1}, // 1.7976931348623158e+308
			{__LINE__, -DBL_MAX, "-180.0e306", 3, 1}, // 1.7976931348623158e+308

			{__LINE__, std::nextafter(0.0, 1), "0.0  ", 3, 1}, // 4.9406564584124654e-324
			{__LINE__, std::nextafter(DBL_TRUE_MIN, 0), "0.0  ", 3, 1},
			{__LINE__, DBL_TRUE_MIN, "0.0  ", 3, 1},
			{__LINE__, std::nextafter(DBL_TRUE_MIN, 1), "0.0  ", 3, 1},
			{__LINE__, DBL_TRUE_MIN * 1'000, "0.0  ", 3, 1},
			{__LINE__, DBL_MIN / 1'000, "0  ", 1, 0},
			{__LINE__, std::nextafter(DBL_MIN, 0), "0  ", 1, 0},
			{__LINE__, DBL_MIN, "20e-309", 1, 0}, // 2.2250738585072014e-308
			{__LINE__, std::nextafter(DBL_MIN, 1), "20e-309", 1, 0},
			{__LINE__, DBL_MIN, "22251.0e-312", 5, 1}, // 2.2250738585072014e-308
			{__LINE__, DBL_MIN * 10, "223.0e-309", 3, 1}, // 2.2250738585072014e-308
			{__LINE__, DBL_MIN * 100, "2.2e-306", 3, 1}, // 2.2250738585072014e-306
			{__LINE__, DBL_MIN * 100, "2.2e-306", 4, 1}, // 2225.0738585072014e-309
			{__LINE__, DBL_MIN * 1000, "22.3e-306", 4, 1}, // 22.250738585072014e-306
			{__LINE__, DBL_MIN / 10, "0  ", 1, 0}, // 2.2250738585072014e-309
			{__LINE__, DBL_MIN / 10, "0.0  ", 5, 1}, // 2.2250738585072014e-309
			{__LINE__, DBL_MIN / 100, "0  ", 1, 0}, // 2.2250738585072014e-310

			{__LINE__, 1e30, "1 Q", 1, 0},
			{__LINE__, 900e30, "900 Q", 1, 0},
			{__LINE__, 900e30, "900.0 Q", 2, 1},
			{__LINE__, 999e30, "1.0e33", 2, 1},
			{__LINE__, 999.9e30, "999.9 Q", 4, 1},
			{__LINE__, 999.9e30, "999900.0 R", 5, 1},
			{__LINE__, 999.99e30, "999990.0 R", 5, 1},

			{__LINE__, DBL_MAX, "200e306", 1, 0}, // 1.7976931348623158e+308
			{__LINE__, -DBL_MAX, "-200e306", 1, 0},
			{__LINE__, DBL_MAX, "179769.30e303", 7, 2},
			{__LINE__, -DBL_MAX, "-179769.30e303", 7, 2},
			{__LINE__, DBL_MAX, "180.0e306", 3, 1},
			{__LINE__, DBL_MAX, "179770.0e303", 5, 1},

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

			{ __LINE__, .0, "0  ", 1, 0 },
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

			{__LINE__, 0.09999999,          "100 m", 1, 0},
			{__LINE__, 0.99999999,            "1  ", 1, 0},
			{__LINE__, 9.99999999,           "10  ", 1, 0},
			{__LINE__, 99.9999999,          "100  ", 1, 0},
			{__LINE__, 999.999999,            "1 k", 1, 0},

			{__LINE__, 0.09999999,        "100.0 m", 2, 1},
			{__LINE__, 0.99999999,          "1.0  ", 2, 1},
			{__LINE__, 9.99999999,         "10.0  ", 2, 1},
			{__LINE__, 99.9999999,        "100.0  ", 2, 1},
			{__LINE__, 999.999999,          "1.0 k", 2, 1},
			{__LINE__, 0.09999999,     "100000.0 u", 5, 1},
			{__LINE__, 0.99999999,       "1000.0 m", 5, 1},
			{__LINE__, 9.99999999,      "10000.0 m", 5, 1},
			{__LINE__, 99.9999999,     "100000.0 m", 5, 1},
			{__LINE__, 999.999999,       "1000.0  ", 5, 1},

			{__LINE__, 0.09999999e-3,  "100.0000 u", 5, 4},
			{__LINE__, 0.99999999e-3,    "1.0000 m", 5, 4},
			{__LINE__, 9.99999999e-3,   "10.0000 m", 5, 4},
			{__LINE__, 99.9999999e-3,  "100.0000 m", 5, 4},
			{__LINE__, 999.999999e-3,    "1.0000  ", 5, 4},
			{__LINE__, 0.09999999,     "100.0000 m", 5, 4},
			{__LINE__, 0.99999999,       "1.0000  ", 5, 4},
			{__LINE__, 9.99999999,      "10.0000  ", 5, 4},
			{__LINE__, 99.9999999,     "100.0000  ", 5, 4},
			{__LINE__, 999.999999,       "1.0000 k", 5, 4},
			{__LINE__, 0.09999999e3,   "100.0000  ", 5, 4},
			{__LINE__, 0.99999999e3,     "1.0000 k", 5, 4},
			{__LINE__, 9.99999999e3,    "10.0000 k", 5, 4},
			{__LINE__, 99.9999999e3,   "100.0000 k", 5, 4},
			{__LINE__, 999.999999e3,     "1.0000 M", 5, 4},

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

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

#include "../vi_timing.h"
#include "internal.h"

#ifdef _WIN32
#	include <Windows.h> // SetThreadAffinityMask
#	include <processthreadsapi.h> // GetCurrentProcessorNumber
#elif defined (__linux__)
#	include <pthread.h> // For pthread_setaffinity_np.
#	include <sched.h> // For sched_getcpu.
#endif

#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace
{
	[[nodiscard]] double round_ext(double num, unsigned char prec)
	{	constexpr auto EPS = std::numeric_limits<decltype(num)>::epsilon();
		if (num > .0 && prec > 0)
		{	const auto exp = static_cast<int>(std::ceil(std::log10(num)));
			assert(-EPS < num * std::pow(10, -exp) && num * std::pow(10, -exp) < 1.0 + EPS);
			const auto factor = std::pow(10, prec - exp);
			num = std::round(num * (1.0 + EPS) * factor) / factor;
		}
		else
		{	assert(num >= .0 && prec > 0);
		}

		return num;
	}

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

void VI_TM_CALL vi_tmThreadAffinityFixate()
{	affinity_fix_t::fixate();
}

void VI_TM_CALL vi_tmThreadAffinityRestore()
{	affinity_fix_t::restore();
}

[[nodiscard]] std::string misc::to_string(misc::duration_t sec, unsigned char prec, unsigned char dec)
{	auto num = sec.count();
	assert(.0 <= num && 0 < prec && 0 <= dec && dec + 3 >= prec);
	num = round_ext(num, prec);

	std::string_view suffix = "ps";
	double factor = 1e12;
	if(1e-12 > num)
	{	num = 0.0;
	}
	else
	{	const auto triple = static_cast<int>(std::floor(std::log10(num))) - 3 * ((prec - dec - 1) / 3);
		if (-12 > triple) { num = 0.0; }
		else if (-9 > triple) { suffix = "ps"; factor = 1e12; }
		else if (-6 > triple) { suffix = "ns"; factor = 1e9; }
		else if (-3 > triple) { suffix = "us"; factor = 1e6; }
		else if (0 > triple) { suffix = "ms"; factor = 1e3; }
		else if (+3 > triple) { suffix = "s "; factor = 1e0; }
		else if (+6 > triple) { suffix = "ks"; factor = 1e-3; }
		else { suffix = "Ms"; factor = 1e-6; }
	}

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(dec) << num * factor << ' ' << suffix;
	return ss.str();
}

void VI_TM_CALL vi_tmWarming(unsigned int threads_qty, unsigned int ms)
{
	if (0 == ms)
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

	for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ ms }; ch::steady_clock::now() < stop;)
	{	load_dummy();
	}

	done = true;

	for (auto &t : additional_threads)
	{	t.join();
	}
}

#ifndef NDEBUG
namespace
{
	const auto nanotest =
		(	[]
			{	const struct
				{	misc::duration_t v_;
					std::string_view expected_;
					unsigned char prec_{3};
					unsigned char dec_{1};
					int line_;
				} vals[] =
				{
					{ 0.0, "0.0 ps", 3, 1, __LINE__ },
					{ 1.2345e12, "1230000.00 Ms", 3, 2, __LINE__ },
					{ 0.555, "560.0 ms", 2, 1, __LINE__ },
					{ 5.55, "5.6 s ", 2, 1, __LINE__ },
					{ 55.5, "56.0 s ", 2, 1, __LINE__ },
				};

				for (auto &v : vals)
				{	const auto s = misc::to_string(v.v_, v.prec_, v.dec_);
					assert(s == v.expected_);
				}
			}(),
			0
		);
}
#endif // #ifndef NDEBUG

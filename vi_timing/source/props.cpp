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
#include "../vi_timing.hpp"

#include <thread>
#include <chrono>

#undef VI_OPTIMIZE_ON
#define VI_OPTIMIZE_ON
#undef VI_OPTIMIZE_OFF
#define VI_OPTIMIZE_OFF

using namespace std::chrono_literals;
namespace ch = std::chrono;

namespace
{
	using duration = ch::duration<double>;

	class thread_affinity_fix_t
	{
		thread_affinity_fix_t(const thread_affinity_fix_t &) = delete;
		thread_affinity_fix_t(thread_affinity_fix_t &&) = delete;
		void operator=(const thread_affinity_fix_t &) = delete;
		void operator=(thread_affinity_fix_t &&) = delete;
	public:
		thread_affinity_fix_t() { vi_tmCurrentThreadAffinityFixate(); }
		~thread_affinity_fix_t() { vi_tmCurrentThreadAffinityRestore(); }
	};

	constexpr auto cache_warmup = 5U;
	constexpr auto now = ch::steady_clock::now;

	duration seconds_per_tick()
	{
		auto time_point = []
		{	std::this_thread::yield(); // To minimize the likelihood of interrupting the thread between measurements.
			for(auto n = cache_warmup + 1; --n;) // Preloading a functions into cache
			{	[[maybe_unused]] volatile auto dummy_1 = vi_tmGetTicks();
				[[maybe_unused]] volatile auto dummy_2 = now();
			}

			// Are waiting for the start of a new time interval.
			auto time = now();
			for (const auto s = time; s == time; time = now())
			{/**/}

			return std::tuple{ vi_tmGetTicks(), time };
		};

		const auto [tick1, time1] = time_point();
		// Load the thread at 100% for 64ms.
		for (auto stop = time1 + 64ms; now() < stop;)
		{/**/}
		const auto [tick2, time2] = time_point();

		return duration(time2 - time1) / (tick2 - tick1);
	}

VI_OPTIMIZE_OFF
	duration measurement_duration()
	{	
		static auto gauge_zero = []
			{	static vi_tmMeasuring_t* const meas_point = vi_tmMeasuring(nullptr, ""); // Create a service item with empty name "".
				const auto start = vi_tmGetTicks();
				const auto finish = vi_tmGetTicks();
				vi_tmMeasuringAdd(meas_point, finish - start, 1U);
			};
		auto time_point = []
			{	std::this_thread::yield(); // To minimize the chance of interrupting the flow between measurements.
				for (auto cnt = cache_warmup + 1; --cnt; )
				{	gauge_zero(); // Preloading a functions into cache
				}

				// Are waiting for the start of a new time interval.
				auto result = now();
				for (const auto s = result; s == result; result = now())
				{/**/}

				return result;
			};

		constexpr auto CNT = 500U;
		auto s = time_point();
		for (auto cnt = CNT + 1; --cnt; )
		{	gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); // 5 calls
		}
		const auto pure = now() - s;

		constexpr auto EXT = 20U;
		s = time_point();
		for (auto cnt = CNT + 1; --cnt; )
		{	gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); // 5 calls

			// EXT calls
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
		}
		const auto dirty = now() - s;

		return duration(dirty - pure) / (EXT * CNT);
	}
VI_OPTIMIZE_ON

VI_OPTIMIZE_OFF
	double measurement_cost()
	{	constexpr auto CNT = 500U;

		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		VI_TM_TICK s;
		for (auto cnt = cache_warmup + 1U; --cnt; )
		{	s = vi_tmGetTicks(); // Preloading a function into cache
		}

		VI_TM_TICK e;
		for (auto cnt = CNT + 1U; --cnt; )
		{	e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks();
		}
		const auto pure = e - s;

		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		for (auto cnt = cache_warmup + 1U; --cnt; )
		{	s = vi_tmGetTicks();
		}

		constexpr auto EXT = 20U;
		for (auto cnt = CNT + 1U; --cnt; )
		{	e = vi_tmGetTicks(); e = vi_tmGetTicks();  e = vi_tmGetTicks(); e = vi_tmGetTicks();

			// EXT calls (Unless optimization is disabled, redundant assignments will be removed, and the function vi_tmGetTicks will be inlined.)
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();

			e = vi_tmGetTicks();
		}
		const auto dirty = e - s;

		return static_cast<double>(dirty - pure) / (EXT * CNT);
	}
VI_OPTIMIZE_ON

VI_OPTIMIZE_OFF
	double resolution()
	{	for (auto CNT = 8;; CNT *= 8) //-V1044 Loop break conditions do not depend on the number of iterations.
		{	VI_TM_TICK first = 0;
			VI_TM_TICK last = 0;

			std::this_thread::yield(); // Reduce the likelihood of interrupting measurements by switching threads.
			const auto limit = now() + 256us;
			for (auto n = cache_warmup + 1; --n; )
			{	first = last = vi_tmGetTicks();
			}

			for (auto cnt = CNT; cnt; )
			{	if (const auto current = vi_tmGetTicks(); current != last)
				{	last = current;
					--cnt;
				}
			}

			if (now() > limit)
			{	return static_cast<double>(last - first) / CNT;
			}
		}
	}
VI_OPTIMIZE_ON
}

misc::properties_t::properties_t()
{	thread_affinity_fix_t thread_affinity_fix_guard;
	std::this_thread::yield();

	vi_tmWarming(1, 500);

	vi_tmJournalReset(nullptr, "");
	seconds_per_tick_ = seconds_per_tick();
	clock_latency_ticks_ = measurement_cost();
	all_latency_ = measurement_duration();
	clock_resolution_ticks_ = resolution();
}

const misc::properties_t& misc::properties_t::props()
{	static const misc::properties_t inst_;
	return inst_;
}

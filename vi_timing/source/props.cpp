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
#include "../vi_timing.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <utility>

#if false
#	undef VI_OPTIMIZE_ON
#	define VI_OPTIMIZE_ON /**/

#	undef VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_OFF /**/
#endif

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

	constexpr auto cache_warmup = 10U;
	constexpr auto now = ch::steady_clock::now;
	using time_point_t = std::invoke_result_t<decltype(now)>;

	duration seconds_per_tick()
	{	// Lambda to get current time and tick after waiting for a new time interval
		auto time_point = []
			{	std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.
				// Preload functions into cache to minimize cold start effects.
				for (auto n = cache_warmup; n; --n)
				{	// I hope that the compiler considers these functions to have side effects and does not discard their calls.
					std::ignore = now();
					std::ignore = vi_tmGetTicks();
				}

				time_point_t time;
				// Wait for the start of a new time interval.
				for (const auto s = time; s == time;)
				{	time = now();
				}
				VI_TM_TICK tick = vi_tmGetTicks();

				return std::pair{ time, tick };
			};

		const auto [s_time, s_ticks] = time_point();
		const auto stop = s_time + 64ms;
		time_point_t f_time;
		VI_TM_TICK f_tick;
		do
		{	std::tie(f_time, f_tick) = time_point();
		}
		while (f_time < stop);

		return (f_tick == s_ticks) ? duration{ 0 } : duration{ f_time - s_time } / (f_tick - s_ticks);
	}

VI_OPTIMIZE_OFF
	duration measurement_duration()
	{	static vi_tmMeasuring_t* const meas_point = vi_tmMeasuring(nullptr, ""); // Create a service item with empty name "".
		static const auto gauge_zero = []
			{	const auto start = vi_tmGetTicks();
				std::atomic_signal_fence(std::memory_order_seq_cst); // Ensure that the measurement is not optimized out.
				const auto finish = vi_tmGetTicks();
				vi_tmMeasuringRepl(meas_point, finish - start, 1U);
			};
		auto time_point = []
			{	std::this_thread::yield(); // To minimize the chance of interrupting the flow between measurements.
				for (auto cnt = cache_warmup; cnt; --cnt)
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
		auto e = now();
		const auto pure = e - s;

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
		e = now();
		const auto dirty = e - s;

		return duration{ dirty - pure } / (EXT * CNT);
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
		{	e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
		}
		const auto pure = e - s;

		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		for (auto cnt = cache_warmup + 1U; --cnt; )
		{	s = vi_tmGetTicks();
		}

		constexpr auto EXT = 20U;
		for (auto cnt = CNT + 1U; --cnt; )
		{	e = vi_tmGetTicks(); e = vi_tmGetTicks();  e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();

			// EXT calls
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
		}
		const auto dirty = e - s;

		return static_cast<double>(dirty - pure) / (EXT * CNT);
	}
VI_OPTIMIZE_ON

VI_OPTIMIZE_OFF
	double resolution()
	{	for (auto CNT = 8;; CNT *= 8) //-V1044 Loop break conditions do not depend on the number of iterations.
		{	volatile VI_TM_TICK first = 0;
			volatile VI_TM_TICK last = 0;

			std::this_thread::yield(); // Reduce the likelihood of interrupting measurements by switching threads.
			const auto limit = now() + 256us;
			for (auto n = cache_warmup; n; --n)
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
	vi_tmJournalReset(nullptr, ""); // Reset a service item with empty name "".
	vi_tmWarming(1, 500);

	seconds_per_tick_ = seconds_per_tick(); // Get the duration of a single tick in seconds.
	clock_latency_ticks_ = measurement_cost(); // Get the cost of a single call of vi_tmGetTicks.
	all_latency_ = measurement_duration(); // Get the cost of a single measurement in seconds.
	clock_resolution_ticks_ = resolution(); // Get the resolution of the clock in ticks.
}

const misc::properties_t& misc::properties_t::props()
{	static const misc::properties_t inst_;
	return inst_;
}

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

#include <thread>
#include <chrono>
#include <memory>

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace
{
	constexpr auto cache_warmup = 5U;
	constexpr auto now = std::chrono::steady_clock::now;

	misc::duration_t seconds_per_tick()
	{
		auto time_point = []
		{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			for(auto n = 0U; n < cache_warmup; ++n) // Preloading a functions into cache
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
		// Load the thread at 100% for 256ms.
		for (auto stop = time1 + 256ms; now() < stop;)
		{/**/}
		const auto [tick2, time2] = time_point();

		return misc::duration_t(time2 - time1) / (tick2 - tick1);
	}

	misc::duration_t measurement_duration()
	{	
		static auto gauge_zero = []
			{	static vi_tmUnit_t* const sheet = vi_tmSheet(nullptr, "");
				const auto start = vi_tmGetTicks();
				const auto finish = vi_tmGetTicks();
				vi_tmRecord(sheet, finish - start, 1);
			};
		auto time_point = []
			{	std::this_thread::yield(); // To minimize the chance of interrupting the flow between measurements.
				for (auto cnt = 0U; cnt < cache_warmup; ++cnt)
				{	gauge_zero(); // Create a service item with empty name "" and cache preload.
				}
				// Are waiting for the start of a new time interval.
				auto result = now();
				for (const auto s = result; s == result; result = now())
				{/**/}
				return result;
			};

		constexpr auto CNT = 500U;
		auto s = time_point();
		for (auto cnt = 0U; cnt < CNT; ++cnt)
		{	gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); // 5 calls
		}
		const auto pure = now() - s;

		constexpr auto EXT = 20U;
		s = time_point();
		for (auto cnt = 0U; cnt < CNT; ++cnt)
		{	gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); // 5 calls

			// EXT calls
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
			gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero(); gauge_zero();
		}
		const auto dirty = now() - s;

		return misc::duration_t(dirty - pure) / (EXT * CNT);
	}

VI_OPTIMIZE_OFF
	double measurement_cost()
	{	constexpr auto CNT = 500U;

		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		auto e = vi_tmGetTicks(); // Preloading a function into cache
		auto s = e;
		for (auto cnt = 0U; cnt < cache_warmup; ++cnt)
		{	s = vi_tmGetTicks();
		}

		for (auto cnt = 0U; cnt < CNT; ++cnt)
		{	e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
		}
		const auto pure = e - s;

		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		for (auto cnt = 0U; cnt < cache_warmup; ++cnt)
		{	s = vi_tmGetTicks();
		}

		constexpr auto EXT = 20U;
		for (auto cnt = 0U; cnt < CNT; ++cnt)
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

	double resolution()
	{	for (auto CNT = 8;; CNT *= 8) //-V1044 Loop break conditions do not depend on the number of iterations.
		{	vi_tmTicks_t first = 0;
			vi_tmTicks_t last = 0;

			std::this_thread::yield(); // Reduce the likelihood of interrupting measurements by switching threads.
			const auto limit = now() + 256us;
			for (auto n = 0U; n < cache_warmup; ++n)
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

const properties_t& props()
{	static const properties_t inst_;
	return inst_;
}

properties_t::properties_t()
{	vi_tm::thread_affinity_fix_t thread_affinity_fix_guard;
	std::this_thread::yield();

	vi_tmWarming(1, 500);

	vi_tmJournalClear(nullptr, "");
	seconds_per_tick_ = seconds_per_tick();
	clock_latency_ = measurement_cost();
	all_latency_ = measurement_duration();
	clock_resolution_ = resolution();
}

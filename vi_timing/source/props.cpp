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

#include <algorithm> // For std::nth_element
#include <array>
#include <cassert>
#include <chrono> // For std::chrono::steady_clock, std::chrono::duration, std::chrono::milliseconds
#include <functional> // For std::invoke_result_t
#include <thread> // For std::this_thread::yield()
#include <utility> // For std::pair, std::index_sequence, std::make_index_sequence, std::forward, std::invoke

using namespace std::chrono_literals;
namespace ch = std::chrono;

namespace
{	const auto now = ch::steady_clock::now;
	using time_point_t = std::invoke_result_t<decltype(now)>;
	using duration_t = ch::duration<double>;

	constexpr auto CACHE_WARMUP = 6U;
	constexpr char SERVICE_NAME[16] = "Bla-bla-bla-bla"; // A service item name for the journal (SSO size).

	template <std::size_t... Is, typename F, typename... Args>
	constexpr auto multiple_invoke_aux(std::index_sequence<Is...>, F&& fn, Args&&... args)
	{	using return_t = std::invoke_result_t<F, Args...>;
		if constexpr (std::is_void_v<return_t>)
		{	((static_cast<void>(Is), std::invoke(fn, args...)), ...);
		}
		else
		{	return_t result{};
			((static_cast<void>(Is), const_cast<volatile return_t &>(result) = std::invoke(fn, args...)), ...);
			return result; // Return the last result.
		}
	}

	template <unsigned N, typename F, typename... Args>
	constexpr auto multiple_invoke(F&& fn, Args&&... args) // Invoke a function N times without overhead costs for organizing the cycle.
	{	static_assert(N > 0);
		return multiple_invoke_aux(std::make_index_sequence<N>{}, fn, std::forward<Args>(args)...);
	}

	template <unsigned N, typename F, typename... Args>
	double diff_calc_aux(F&& fn, Args&&... args)
	{	constexpr auto MULTIPLIER = 128U;
		auto start = []
			{	VI_TM_TICK result;
				const auto prev = vi_tmGetTicks();
				do
				{	result = vi_tmGetTicks();
				} while (prev == result); // Wait for the start of a new time interval.
				return result;
			};
		auto func = multiple_invoke<N, F, Args...>;
		std::array<VI_TM_TICK, 9> diff;

		for (auto &d : diff)
		{	std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.
			for (auto cw = 0U; cw < CACHE_WARMUP; cw++) // Preload functions into cache to minimize cold start effects.
			{	(void)start();
				(void)func(fn, args...);
			}

			const auto s = start();
			for (auto rpt = 0U; rpt < MULTIPLIER; rpt++)
			{	func(fn, args...);
			}
			const auto f = vi_tmGetTicks();
			d = f - s;
		}

		auto const mid = diff.begin() + diff.size() / 2;
		std::nth_element(diff.begin(), mid, diff.end());
		return static_cast<double>(*mid) / MULTIPLIER;
	}

	template <typename F, typename... Args>
	double diff_calc(F&& fn, Args&&... args)
	{	constexpr auto BASE = 4U;
		constexpr auto FULL = BASE + 32U;
		const double base = diff_calc_aux<BASE>(fn, args...);
		const double full = diff_calc_aux<FULL>(fn, args...);
		return (full - base) / (FULL - BASE);
	}

	VI_NOINLINE void measuring(VI_TM_HJOUR journal, const char* name)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		const auto h = vi_tmMeasuring(journal, name);
		vi_tmMeasuringRepl(h, finish - start, 1U);
	};

	VI_NOINLINE void measuring_with_caching(VI_TM_HMEAS m)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		vi_tmMeasuringRepl(m, finish - start, 1U);
	};

	VI_NOINLINE auto meas_GetTicks()
	{	return vi_tmGetTicks();
	}

	inline
	std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)>
	create_journal()
	{	return { vi_tmJournalCreate(), &vi_tmJournalClose };
	}

	duration_t meas_seconds_per_tick()
	{
		auto start = []
			{	auto result{ now() };
				// Wait for the start of a new time interval.
				for (const auto s = result; s == result; result = now())
				{/**/}
				return result;
			};

		time_point_t c_time;
		VI_TM_TICK c_ticks;
		auto const s_time = start();
		auto const s_ticks = vi_tmGetTicks();
		auto const stop = s_time + 1ms;
		do
		{	c_time = start();
			c_ticks = vi_tmGetTicks();
		}
		while (c_time < stop || c_ticks - s_ticks < 10);

		return duration_t{ c_time - s_time } / (c_ticks - s_ticks);
	}

	VI_NOINLINE double meas_resolution_aux()
	{	constexpr auto N = 16U;
		auto last = vi_tmGetTicks();
		const auto first = last;
		for (auto cnt = N; cnt; )
		{	if (const auto current = vi_tmGetTicks(); current != last)
			{	last = current;
				--cnt;
			}
		}
		return static_cast<double>(last - first) / N;
	}

	double meas_resolution()
	{	std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.
		return multiple_invoke<CACHE_WARMUP + 1>(meas_resolution_aux);
	}

	auto meas_cost_calling_tick_function()
	{	return diff_calc(meas_GetTicks);
	}

	auto meas_duration()
	{	double result{};
		if (auto journal = create_journal())
		{	result = diff_calc(measuring, journal.get(), SERVICE_NAME);
		}
		return result;
	}

	auto meas_duration_with_caching()
	{	double result{};
		if (const auto journal = create_journal())
		{	if (const auto m = vi_tmMeasuring(journal.get(), SERVICE_NAME))
			{	result = diff_calc(measuring_with_caching, m);
			}
		}
		return result;
	}
} // namespace

misc::properties_t::properties_t()
{
	struct thread_affinity_fix_guard_t // RAII guard to fixate the current thread's affinity.
	{	thread_affinity_fix_guard_t() { vi_tmCurrentThreadAffinityFixate(); }
		~thread_affinity_fix_guard_t() { vi_tmCurrentThreadAffinityRestore(); }
	} thread_affinity_fix_guard; // Fixate the current thread's affinity to avoid issues with clock resolution measurement.

	vi_tmWarming(1);

	clock_resolution_ticks_ = meas_resolution(); // The resolution of the clock in ticks.
	seconds_per_tick_ = meas_seconds_per_tick(); // The duration of a single tick in seconds.
	clock_latency_ticks_ = meas_cost_calling_tick_function(); // The cost of a single call of vi_tmGetTicks.
	duration_ = seconds_per_tick_ * meas_duration_with_caching(); // The cost of a single measurement with preservation in seconds.
	duration_ex_ = seconds_per_tick_ * meas_duration(); // The cost of a single measurement in seconds.
}

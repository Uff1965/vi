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
#include <iterator>
#include <thread> // For std::this_thread::yield()
#include <utility> // For std::pair, std::index_sequence, std::make_index_sequence, std::forward, std::invoke

using namespace std::chrono_literals;
namespace ch = std::chrono;

namespace
{	constexpr auto now = ch::steady_clock::now;
	using time_point_t = std::invoke_result_t<decltype(now)>;

	constexpr auto CACHE_WARMUP = 6U;
	constexpr char SERVICE_NAME[] = "Bla-bla-bla-bla"; // A service item name for the journal (SSO size).
	constexpr char const* sandbox_names[] =
	{	"foo", "bar", "baz", "qux", "quux",
		"corge", "grault", "garply", "waldo", "fred",
		"plugh", "xyzzy", "thud", "hoge", "fuga",
	};

	auto start_tick()
	{	VI_TM_TICK result;
		const auto prev = vi_tmGetTicks();
		do
		{	result = vi_tmGetTicks();
		} while (prev == result); // Wait for the start of a new time interval.
		return result;
	}

	auto start_now()
	{	time_point_t result;
		const auto prev = now();
		do
		{	result = now();
		} while (prev == result); // Wait for the start of a new time interval.
		return result;
	}

	std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)>
	create_journal()
	{	return { vi_tmJournalCreate(), &vi_tmJournalClose };
	}

	template <auto F, typename... Args, std::size_t... Is>
	constexpr auto multiple_invoke_aux(std::index_sequence<Is...>, Args&&... args)
	{	using return_t = std::invoke_result_t<decltype(F), Args...>;
		if constexpr (std::is_void_v<return_t>)
		{	((static_cast<void>(Is), std::invoke(F, args...)), ...);
		}
		else
		{	volatile return_t results[] = {(static_cast<void>(Is), std::invoke(F, args...))...};
			return results[sizeof...(Is) - 1U]; // Return the last result.
		}
	}

	// Invoke a F function N times without overhead costs for organizing the cycle.
	template <unsigned N, auto F, typename... Args>
	constexpr auto multiple_invoke(Args&&... args)
	{	static_assert(N > 0);
		return multiple_invoke_aux<F, Args...>(std::make_index_sequence<N>{}, std::forward<Args>(args)...);
	}

	template<typename It>
	auto median(It b, It e)
	{	const auto n = e - b;
		assert(n > 0);
		auto mid = b + n / 2U;
		std::nth_element(b, mid, e);

		return (n % 2U) != 0 ? *mid : (*mid + *std::max_element(b, mid)) / 2U;
	}

	template <unsigned N, auto F, typename... Args>
	double calc_diff_ticks_aux(Args&&... args)
	{	constexpr auto REPEAT = 512U;
		constexpr auto SIZE = 31U;

		std::array<VI_TM_TICK, SIZE + CACHE_WARMUP> diff;
		std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.
		for (auto &d : diff)
		{	const auto s = start_tick();
			for (auto rpt = 0U; rpt < REPEAT; rpt++)
			{	multiple_invoke<N, F, Args...>(args...);
			}
			const auto f = vi_tmGetTicks();
			d = f - s;
		}

		// First CACHE_WARMUP elements are for warming up the cache, so we ignore them.
		// Obtain the median value among the remaining ones.
		return static_cast<double>(median(diff.begin() + CACHE_WARMUP, diff.end())) / static_cast<double>(REPEAT);
	}

	template <auto F, typename... Args>
	double calc_diff_ticks(Args&&... args)
	{	constexpr auto BASE = 2U;
		constexpr auto EXTRA = 5U;
		const double full = calc_diff_ticks_aux<BASE + EXTRA, F>(args...);
		const double base = calc_diff_ticks_aux<BASE, F>(args...);
		return (full - base) / static_cast<double>(EXTRA);
	}

	void body_duration(VI_TM_HJOUR journal, const char* name)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		const auto h = vi_tmMeasurement(journal, name);
		vi_tmMeasurementAdd(h, finish - start, 1U);
	};

	void body_threadsafe_measuring_with_caching(VI_TM_HMEAS m)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		vi_tmMeasurementAdd(m, finish - start, 1U);
	};

	double meas_resolution()
	{	constexpr auto N = 8U;
		constexpr auto SIZE = 17U;
		std::array<VI_TM_TICK, SIZE + CACHE_WARMUP> arr;
		std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.
		for (auto &item : arr)
		{	const auto first = vi_tmGetTicks();
			auto last = first;
			for (auto cnt = N; cnt; )
			{	if (const auto current = vi_tmGetTicks(); current != last)
				{	last = current;
					--cnt;
				}
			}
			item = last - first;
		}
		// First CACHE_WARMUP elements are for warming up the cache, so we ignore them.
		return static_cast<double>(median(arr.begin() + CACHE_WARMUP, arr.end())) / static_cast<double>(N);
	}

	auto meas_seconds_per_tick()
	{
		time_point_t c_time;
		VI_TM_TICK c_ticks;
		auto const s_time = start_now();
		auto const s_ticks = vi_tmGetTicks();
		auto const stop = s_time + 10ms;
		do
		{	c_time = start_now();
			c_ticks = vi_tmGetTicks();
		}
		while (c_time < stop || c_ticks - s_ticks < 10);

		return ch::duration<double>{ c_time - s_time } / (c_ticks - s_ticks);
	}

	auto meas_cost_calling_tick_function()
	{	return calc_diff_ticks<vi_tmGetTicks>();
	}

	auto meas_threadsafe_duration_with_caching()
	{	double result{};
		if (const auto journal = create_journal())
		{	if (const auto m = vi_tmMeasurement(journal.get(), SERVICE_NAME))
			{	result = calc_diff_ticks<body_threadsafe_measuring_with_caching>(m);
			}
		}
		return result;
	}

	auto meas_duration()
	{	double result{};
		if (auto journal = create_journal())
		{	(void)vi_tmMeasurement(journal.get(), SERVICE_NAME);
			for (auto n : sandbox_names)
			{	(void)vi_tmMeasurement(journal.get(), n);
			}
			
			result = calc_diff_ticks<body_duration>(journal.get(), SERVICE_NAME);
		}
		return result;
	}
} // namespace

const misc::properties_t&
misc::properties_t::props()
{	static const properties_t self;
	return self;
}

misc::properties_t::properties_t()
{
	struct thread_affinity_fix_guard_t // RAII guard to fixate the current thread's affinity.
	{	thread_affinity_fix_guard_t() { vi_CurrentThreadAffinityFixate(); }
		~thread_affinity_fix_guard_t() { vi_CurrentThreadAffinityRestore(); }
	} thread_affinity_fix_guard; // Fixate the current thread's affinity to avoid issues with clock resolution measurement.

	vi_Warming(1, 500);

	clock_resolution_ticks_ = meas_resolution(); // The resolution of the clock in ticks.
	seconds_per_tick_ = meas_seconds_per_tick(); // The duration of a single tick in seconds.
	clock_overhead_ticks_ = meas_cost_calling_tick_function(); // The cost of a single call of vi_tmGetTicks.
	duration_threadsafe_ = seconds_per_tick_ * meas_threadsafe_duration_with_caching(); // The cost of a single measurement with preservation in seconds.
	duration_ex_threadsafe_ = seconds_per_tick_ * meas_duration(); // The cost of a single measurement in seconds.
}

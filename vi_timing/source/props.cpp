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

#include <cassert>
#include <chrono> // For std::chrono::steady_clock, std::chrono::duration, std::chrono::milliseconds
#include <functional> // For std::invoke_result_t
#include <thread> // For std::this_thread::yield()
#include <utility> // For std::pair, std::index_sequence, std::make_index_sequence, std::forward, std::invoke

using namespace std::chrono_literals;
namespace ch = std::chrono;

namespace
{	constexpr char SERVICE_NAME[16] = "Bla-bla-bla-bla"; // A service item name for the journal.
	constexpr auto RPT = 128U; // The number of repetitions for each measurement to reduce the impact of noise.
	constexpr auto BASE = 4U;
	constexpr auto EXT = 32U;

	const auto now = ch::steady_clock::now;
	using time_point_t = std::invoke_result_t<decltype(now)>;
	using duration_t = ch::duration<double>;

	template <typename F, typename... Args>
	auto cache_warming_and_invoke(F&& fn, Args&&... args) // Preload functions into cache to minimize cold start effects.
	{	constexpr auto CACHE_WARMUP = 10U;
		using return_t = std::invoke_result_t<F, Args...>;

		std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.

		if constexpr (std::is_void_v<return_t> )
		{	for (auto n = 0U; n < CACHE_WARMUP; n++)
			{	std::invoke(fn, args...);
			}
		}
		else
		{	return_t result;
			for (auto n = 0U; n < CACHE_WARMUP; n++)
			{	// volatile variable cannot be initialized by an STL object.
				// const_cast<volatile return_t&>(result) = std::invoke(fn, args...);
				result = std::invoke(fn, args...); // TODO: The optimizer will probably remove the assignments.
			}
			return result; // Return the last result.
		}
	}

	template <template <unsigned, unsigned> typename F, typename... Args>
	auto diff_calc(Args&&... args)
	{	const auto base = cache_warming_and_invoke(F<BASE, RPT>::call, args...);
		const auto full = cache_warming_and_invoke(F<BASE + EXT, RPT>::call, args...);
		return (full - base) / (EXT * RPT);
	}

	template <std::size_t... Is, typename F, typename... Args>
	constexpr auto multiple_invoke_impl(std::index_sequence<Is...>, F &fn, Args&&... args)
	{	using return_t = std::invoke_result_t<F, Args...>;
		if constexpr (std::is_void_v<return_t>)
		{	((static_cast<void>(Is), std::invoke(fn, args...)), ...);
		}
		else
		{	return_t result{};
			((static_cast<void>(Is), const_cast<volatile return_t&>(result) = std::invoke(fn, args...)), ...);
			return result; // Return the last result.
		}
	}

	template <unsigned N, typename F, typename... Args>
	constexpr auto multiple_invoke(F &&fn, Args&&... args) // Invoke a function N times without overhead costs for organizing the cycle.
	{	static_assert(N > 0);
		return multiple_invoke_impl(std::make_index_sequence<N>{}, std::forward<F>(fn), std::forward<Args>(args)...);
	}

	auto time_point()
	{	std::pair result{ vi_tmGetTicks(), now() };
		// Wait for the start of a new time interval.
		for (const auto s = result.second; s == result.second;)
		{	result = { vi_tmGetTicks(), now() };
		}
		return result;
	};

	void gauge_zero(VI_TM_HMEAS m)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		vi_tmMeasuringRepl(m, finish - start, 1U);
	};

	void gauge_zero_ex(VI_TM_HJOUR journal, const char* name)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		vi_tmMeasuringRepl(vi_tmMeasuring(journal, name), finish - start, 1U);
	};

	template<unsigned N, unsigned M>
	struct meas_cost_t
	{	static VI_NOINLINE auto call()
		{	auto s = vi_tmGetTicks();
			auto f = s;
			for (auto n = M; n; --n)
			{	f = multiple_invoke<N>(vi_tmGetTicks);
			}
			return f - s;
		}
	};

	template<unsigned N, unsigned M>
	struct meas_duration_t
	{	static VI_NOINLINE auto call(VI_TM_HMEAS m)
		{	auto s = now();
			for (auto n = M; n; --n)
			{	multiple_invoke<N>(gauge_zero, m);
			}
			auto f = now();
			return f - s;
		}
	};

	template<unsigned N, unsigned M>
	struct meas_duration_ex_t
	{	static VI_NOINLINE auto call(VI_TM_HJOUR journal, const char *name)
		{	auto s = now();
			for (auto n = M; n; --n)
			{	multiple_invoke<N>(gauge_zero_ex, journal, name);
			}
			auto f = now();
			return f - s;
		}
	};

	inline std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> create_journal()
	{	return { vi_tmJournalCreate(), &vi_tmJournalClose };
	}

	duration_t meas_seconds_per_tick()
	{	auto f = time_point();
		auto const [s_ticks, s_time] = f;
		auto const stop = s_time + 1ms;
		do
		{	f = time_point();
		}
		while (f.second < stop || f.first - s_ticks < 10);

		return duration_t{ f.second - s_time } / (f.first - s_ticks);
	}

	double meas_resolution()
	{	for (auto N = 8U;; N *= 8U) //-V1044
		{	const auto limit = now() + 256us;
			const auto first = vi_tmGetTicks();
			auto last = first;
			for (auto cnt = N; cnt; )
			{	if (const auto current = vi_tmGetTicks(); current != last)
				{	last = current;
					--cnt;
				}
			}

			if (now() > limit)
			{	return static_cast<double>(last - first) / N;
			}
		}
	}

	double meas_cost()
	{	return diff_calc<meas_cost_t>();
	}

	duration_t meas_duration()
	{	duration_t result{};
		if (const auto journal = create_journal())
		{	if (const auto m = vi_tmMeasuring(journal.get(), SERVICE_NAME))
			{	result = diff_calc<meas_duration_t>(m);
			}
		}
		return result;
	}

	duration_t meas_duration_ex()
	{	duration_t result{};
		if (auto journal = create_journal())
		{	result = diff_calc<meas_duration_ex_t>(journal.get(), SERVICE_NAME);
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
	clock_latency_ticks_ = meas_cost(); // The cost of a single call of vi_tmGetTicks.
	duration_ = meas_duration(); // The cost of a single measurement with preservation in seconds.
	duration_ex_ = meas_duration_ex(); // The cost of a single measurement in seconds.
}

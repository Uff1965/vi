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
#include <functional>
#include <thread>
#include <utility>

#if true
#	undef VI_OPTIMIZE_ON
#	define VI_OPTIMIZE_ON

#	undef VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_OFF
#endif

using namespace std::chrono_literals;
namespace ch = std::chrono;

namespace
{
	namespace detail
	{
		using duration = ch::duration<double>;

		class thread_affinity_fix_t
		{	thread_affinity_fix_t(const thread_affinity_fix_t &) = delete;
			void operator=(const thread_affinity_fix_t &) = delete;
		public:
			thread_affinity_fix_t() { vi_tmCurrentThreadAffinityFixate(); }
			~thread_affinity_fix_t() { vi_tmCurrentThreadAffinityRestore(); }
		};

		const auto now = ch::steady_clock::now;
		using time_point_t = std::invoke_result_t<decltype(now)>;

		inline auto tick_time() noexcept
		{	return std::pair{ vi_tmGetTicks(), now() };
		}

		template <typename F, typename... Args>
		auto cache_warming(F&& fn, Args&&... args) // Preload functions into cache to minimize cold start effects.
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

		template <std::size_t... Is, typename F, typename... Args>
		constexpr auto multiple_invoke_impl(std::index_sequence<Is...>, F &&fn, Args&&... args)
		{	using return_t = std::invoke_result_t<F, Args...>;

			if constexpr (std::is_void_v<return_t>) {
				((static_cast<void>(Is), std::invoke(fn, args...)), ...);
			}
			else
			{	return_t result{};
				((static_cast<void>(Is), const_cast<volatile return_t&>(result) = std::invoke(fn, args...)), ...);
				return result; // Return the last result.
			}
		}

		template <std::size_t N, typename F, typename... Args>
		constexpr auto multiple_invoke(F &&fn, Args&&... args)
		{	static_assert(N > 0);
			return multiple_invoke_impl
			(	std::make_index_sequence<N>{},
				std::forward<F>(fn),
				std::forward<Args>(args)...
			);
		}

		auto time_point()
		{
			auto result = tick_time();

			// Wait for the start of a new time interval.
			for (const auto s = result.second; s == result.second;)
			{
				result = tick_time();
			}

			return result;
		};
	}

	constexpr auto CNT = 256U;
	constexpr auto BASE = 4U;
	constexpr auto EXT = 64U;

	detail::duration meas_seconds_per_tick()
	{	auto f = detail::cache_warming(detail::time_point);
		auto const [s_ticks, s_time] = f;
		auto const stop = s_time + 64ms;
		do
		{	f = detail::time_point();
		}
		while (f.second < stop);

		return (f.first == s_ticks) ?
			detail::duration{ 0 } :
			detail::duration{ f.second - s_time } / (f.first - s_ticks);
	}

	double meas_resolution()
	{	for (auto CNT = 8U;; CNT *= 8U) //-V1044 Loop break conditions do not depend on the number of iterations.
		{	const auto limit = detail::now() + 256us;

			const VI_TM_TICK first = detail::cache_warming(vi_tmGetTicks);
			auto last = first;

			for (auto cnt = CNT; cnt; )
			{	if (const auto current = vi_tmGetTicks(); current != last)
				{	last = current;
					--cnt;
				}
			}

			if (detail::now() > limit)
			{	return static_cast<double>(last - first) / CNT;
			}
		}
	}

VI_OPTIMIZE_OFF
	double meas_cost()
	{	auto s = detail::cache_warming(vi_tmGetTicks);
		auto e = s;
		for (auto cnt = CNT; cnt; --cnt)
		{	e = detail::multiple_invoke<BASE>(vi_tmGetTicks);
		}
		const auto pure = e - s;

		s = detail::cache_warming(vi_tmGetTicks);
		for (auto cnt = CNT; cnt; --cnt)
		{	e = detail::multiple_invoke<BASE + EXT>(vi_tmGetTicks); // + EXT calls
		}
		const auto dirty = e - s;

		return static_cast<double>(dirty - pure) / (EXT * CNT);
	}
VI_OPTIMIZE_ON

VI_OPTIMIZE_OFF
	detail::duration meas_duration()
	{	static vi_tmMeasuring_t* const service_item = vi_tmMeasuring(nullptr, ""); // Get/Create a service item with empty name "".
		static const auto gauge_zero = []
			{	const auto start = vi_tmGetTicks();
				const auto finish = vi_tmGetTicks();
				vi_tmMeasuringRepl(service_item, finish - start, 1U);
			};
		detail::cache_warming([] { (void)gauge_zero(); (void)detail::now(); }); // Preload the functions into cache to minimize cold start effects.

		auto s = detail::now();
		for (auto cnt = CNT + 1; --cnt; )
		{	detail::multiple_invoke<BASE>(gauge_zero);
		}
		auto e = detail::now();
		const auto pure = e - s;

		s = detail::now();
		for (auto cnt = CNT + 1; --cnt; )
		{	detail::multiple_invoke<BASE + EXT>(gauge_zero); // + EXT calls
		}
		e = detail::now();
		const auto dirty = e - s;

		return detail::duration{ dirty - pure } / (EXT * CNT);
	}
VI_OPTIMIZE_ON
}

misc::properties_t::properties_t()
{	detail::thread_affinity_fix_t thread_affinity_fix_guard;
	vi_tmJournalReset(nullptr, ""); // Reset a service item with empty name "".
	vi_tmWarming(1, 500);

	seconds_per_tick_ = meas_seconds_per_tick(); // Get the duration of a single tick in seconds.
	clock_latency_ticks_ = meas_cost(); // Get the cost of a single call of vi_tmGetTicks.
	all_latency_ = meas_duration(); // Get the cost of a single measurement in seconds.
	clock_resolution_ticks_ = meas_resolution(); // Get the resolution of the clock in ticks.
}

const misc::properties_t& misc::properties_t::props()
{	static const misc::properties_t inst_;
	return inst_;
}

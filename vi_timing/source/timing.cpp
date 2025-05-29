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

#include "../vi_timing.h"
#include "build_number_maker.h" // For build number generation.

#include <atomic>
#include <cassert>
#include <cstdint> // std::uint64_t, std::size_t
#include <cmath> // std::sqrt
#include <memory> // std::unique_ptr
#include <mutex> // std::mutex, std::lock_guard
#include <numeric> // std::accumulate
#include <string> // std::string
#include <unordered_map> // unordered_map: "does not invalidate pointers or references to elements".

#ifdef __STDC_NO_ATOMICS__
//	At the moment Atomics are available in Visual Studio 2022 with the /experimental:c11atomics flag.
//	"we left out support for some C11 optional features such as atomics" [Microsoft
//	https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#	error "Atomic objects and the atomic operation library are not supported."
#endif

namespace
{
	inline bool verify(bool b) { assert(b); return b; }

	struct measuring_t
	{
#ifdef VI_TM_STAT_USE_WELFORD
		// Welford’s online algorithm for computing variance
		mutable std::mutex mutex_;
		double mean_ = 0.0; // Mean value.
		double m2_ = 0.0; // Sum of squares.
		size_t cnt_ = 0U; // Number of calls processed.
		VI_TM_TDIFF sum_ = 0U; // Sum of all measurements. Not filtered!
		size_t calls_ = 0U; // Number of calls to 'add()'. Not filtered!
		size_t amt_ = 0U; // Number of items for measurements. Not filtered!
#else
		// Simple statistics: no standard deviation, only the mean value.
		std::atomic<VI_TM_TDIFF> sum_ = 0U;
		std::atomic<size_t> calls_ = 0U;
		std::atomic<size_t> amt_ = 0U;
#endif

		void add(VI_TM_TDIFF diff, size_t amt) noexcept
		{	if(verify(!!amt))
			{
#ifdef VI_TM_STAT_USE_WELFORD
				static constexpr double K = 2.5; // Threshold for outliers.
				std::lock_guard lg(mutex_);
				calls_++;
				sum_ += diff;
				amt_ += amt;
				if (auto d = static_cast<double>(diff) / amt - mean_; cnt_ <= 2 || d < K * std::sqrt(m2_ / cnt_)) // Avoids outliers.
				{	const size_t total = cnt_ + amt;
					mean_ = (cnt_ * mean_ + diff) / total;
					m2_ += d * d * cnt_ * amt / total;
					cnt_ = total;
				}
#else
				calls_.fetch_add(1, std::memory_order_relaxed);
				sum_.fetch_add(diff, std::memory_order_relaxed);
				amt_.fetch_add(amt, std::memory_order_relaxed);
#endif
			}
		}
		vi_tmMeasuringRAW_t get() const noexcept
		{	vi_tmMeasuringRAW_t result;
#ifdef VI_TM_STAT_USE_WELFORD
			std::lock_guard lg(mutex_);
			result.mean_ = mean_;
			result.m2_ = m2_;
			result.cnt_ = cnt_;
#endif
			result.sum_ = sum_;
			result.amt_ = amt_;
			result.calls_ = calls_;
			return result;
		}
		void reset() noexcept
		{
#ifdef VI_TM_STAT_USE_WELFORD
			std::lock_guard lg(mutex_);
			mean_ = m2_ = 0.0;
			cnt_ = 0U;
#endif
			sum_ = 0U;
			amt_ = calls_ = 0U;
		}
	};

	constexpr auto MAX_LOAD_FACTOR = 0.7F;
	constexpr size_t DEFAULT_STORAGE_CAPACITY = 64U;
	using storage_t = std::unordered_map<std::string, measuring_t>;
} // namespace

struct vi_tmMeasuring_t: storage_t::value_type {/**/};
static_assert(sizeof(vi_tmMeasuring_t) == sizeof(storage_t::value_type), "'vi_tmMeasuring_t' should simply be a synonym for 'storage_t::value_type'.");

struct vi_tmJournal_t
{
private:
	std::mutex storage_guard_;
	storage_t storage_;
public:
	static auto& from_handle(VI_TM_HJOUR journal) { static vi_tmJournal_t global; return journal ? *journal : global; }
	explicit vi_tmJournal_t() { storage_.max_load_factor(MAX_LOAD_FACTOR); storage_.reserve(DEFAULT_STORAGE_CAPACITY); }
	int init() { return 0; }
	auto& at(const char *name) { std::lock_guard lock{ storage_guard_ }; return *storage_.try_emplace(name).first; }
	int for_each_measurement(vi_tmMeasEnumCallback_t fn, void *data);
	void reset(const char *name = nullptr);
};

int vi_tmJournal_t::for_each_measurement(vi_tmMeasEnumCallback_t fn, void *data)
{	std::lock_guard lock{ storage_guard_ };
	for (auto &it : storage_)
	{
#if defined VI_TM_STAT_USE_WELFORD
		assert(it.second.amt_ >= it.second.calls_ && (!!it.second.sum_ == !!it.second.calls_) && ((0.0 != it.second.mean_) == !!it.second.cnt_));
#else
		assert(it.second.amt_ >= it.second.calls_);
#endif
		if (!it.first.empty())
		{	if (const auto interrupt = fn(static_cast<VI_TM_HMEAS>(&it), data))
			{	return interrupt;
			}
		}
	}
	return 0;
}

void vi_tmJournal_t::reset(const char *name)
{	std::lock_guard lock{ storage_guard_ };
	if (!name)
	{	for (auto &[dummy_size, item] : storage_)
		{	item.reset();
		}
	}
	else if (const auto [it, b] = storage_.try_emplace(name); !b)
	{	it->second.reset();
	}
}

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
int VI_TM_CALL vi_tmInit()
{	return vi_tmJournal_t::from_handle(nullptr).init();
}

void VI_TM_CALL vi_tmFinit(void)
{	vi_tmJournal_t::from_handle(nullptr).reset();
}

VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate()
{	try
	{	return new vi_tmJournal_t{};
	}
	catch (const std::bad_alloc &)
	{	assert(false);
		return nullptr;
	}
}

void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOUR journal)
{	delete journal;
}

void VI_TM_CALL vi_tmJournalReset(VI_TM_HJOUR journal, const char* name) noexcept
{	vi_tmJournal_t::from_handle(journal).reset(name);
}

int VI_TM_CALL vi_tmMeasuringEnumerate(VI_TM_HJOUR journal, vi_tmMeasEnumCallback_t fn, void *data)
{	return vi_tmJournal_t::from_handle(journal).for_each_measurement(fn, data);
}

VI_TM_HMEAS VI_TM_CALL vi_tmMeasuring(VI_TM_HJOUR journal, const char *name)
{	return static_cast<VI_TM_HMEAS>(&vi_tmJournal_t::from_handle(journal).at(name));
}

void VI_TM_CALL vi_tmMeasuringRepl(VI_TM_HMEAS meas, VI_TM_TDIFF tick_diff, size_t amount) noexcept
{	if (verify(meas)) { meas->second.add(tick_diff, amount); }
}

void VI_TM_CALL vi_tmMeasuringGet(VI_TM_HMEAS meas, const char* *name, vi_tmMeasuringRAW_t *data)
{	if (verify(meas))
	{	if (name) { *name = meas->first.c_str(); }
		if (data) { *data = meas->second.get(); }
	}
}

void VI_TM_CALL vi_tmMeasuringReset(VI_TM_HMEAS meas)
{	if (verify(meas)) { meas->second.reset(); }
}
//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifndef NDEBUG
namespace
{
	const auto nanotest = []
		{
#	if false
			static constexpr VI_TM_TDIFF samples_simple[] = { 34, 81, 17 };
			static constexpr VI_TM_TDIFF samples_multiple[] = { 34, };
			static constexpr auto M = 2;
#	else
			static constexpr VI_TM_TDIFF samples_simple[] =
			{	46, 52, 50, 49, 51, 50, 48, 48, 52, 46, 49, 51, 49, 52, 54, 47, 48, 51, 46, 49,
				51, 48, 52, 50, 50, 49, 51, 51, 53, 51, 48, 51, 52, 52, 49, 53, 52, 48, 46, 49,
				51, 52, 52, 47, 50, 49, 50, 52, 47, 54, 51, 48, 49, 51, 51, 49, 50, 53, 50, 49,
				50, 51, 50, 45, 51, 51, 52, 49, 50, 49, 50, 51, 53, 49, 51, 48, 49, 52, 52, 50,
				53, 47, 52, 52, 50, 46, 49, 50, 49, 51, 50, 50, 51, 49, 49, 52, 48, 50, 47, 53,
				49, 48, 48, 52, 53, 53, 49, 52, 52, 53, 48, 50, 49, 51, 51, 46, 49, 52, 47, 48,
				50, 50, 50, 51, 48, 49, 52, 51, 51, 46, 50, 48, 50, 48, 48, 51, 52, 53, 51, 47,
				48, 51, 49, 48, 47, 50, 52, 50, 53, 52, 49, 48, 53, 53, 47, 52, 48, 48, 51, 53,
				49, 50, 49, 49, 50, 52, 49, 49, 53, 51, 48, 50, 51, 48, 53, 46, 50, 48, 48, 50,
				52, 52, 51, 49, 52, 46, 49, 49, 49, 48, 49, 49, 49, 48, 52, 52, 52, 51, 54, 48,
			};
			static constexpr VI_TM_TDIFF samples_multiple[] = { 50, 45, 52, 48, 50, 50, 47, 46, 46, 49, };
			static constexpr auto M = 10;
#	endif
			static constexpr VI_TM_TDIFF samples_exclue[] = { 93, 98, 97, 100, 99, 99, 110, 97, 100, 98, };

			static constexpr auto n = std::size(samples_simple) + M * std::size(samples_multiple);
			static const auto mean = 
				(	std::accumulate(std::cbegin(samples_simple), std::cend(samples_simple), 0.0) +
					M * std::accumulate(std::cbegin(samples_multiple), std::cend(samples_multiple), 0.0)
				) / n;
			static const auto S = []
				{	const auto m2 = std::accumulate
					(	std::cbegin(samples_simple),
						std::cend(samples_simple),
						0.0,
						[](auto i, auto v) { const auto d = v - mean; return i + d * d; }
					) + M * std::accumulate
					(	std::cbegin(samples_multiple),
						std::cend(samples_multiple),
						0.0,
						[](auto i, auto v) { const auto d = v - mean; return i + d * d; }
					);
					return std::sqrt(m2 / (n - 1));
				}();
			static constexpr char NAME[] = "dummy";

			const char *name = nullptr;
			vi_tmMeasuringRAW_t md;
			std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> journal{ vi_tmJournalCreate(), vi_tmJournalClose };
			{	const auto m = vi_tmMeasuring(journal.get(), NAME);
				for (auto x : samples_simple)
				{	vi_tmMeasuringRepl(m, x);
				}
				for (auto x : samples_multiple)
				{	vi_tmMeasuringRepl(m, M * x, M);
				}
#	ifdef VI_TM_STAT_USE_WELFORD
				for (auto x : samples_exclue)
				{	vi_tmMeasuringRepl(m, x, 1);
				}
#	endif
				vi_tmMeasuringGet(m, &name, &md);
			}

			assert(name && std::strlen(name) + 1 == std::size(NAME) && 0 == std::strcmp(name, NAME));
#	ifdef VI_TM_STAT_USE_WELFORD
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple) + std::size(samples_exclue));
			assert(std::abs(md.mean_ - mean) / mean < 1e-12);
			const auto s = std::sqrt(md.m2_ / (md.cnt_ - 1));
			assert(std::abs(s - S) / S < 1e-12);
#	else
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple));
			assert(std::abs(static_cast<double>(md.sum_) / md.amt_ - mean) < 1e-12);
#	endif
			return 0;
		}();
}
#endif // #ifndef NDEBUG

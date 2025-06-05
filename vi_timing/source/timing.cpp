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

#include <atomic> // std::atomic
#include <cassert> // assert()
#include <cmath> // std::sqrt
#include <cstdint> // std::uint64_t, std::size_t
#include <functional> // std::invoke
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
		// Welford’s online algorithm for computing variance and filtering of outliers.
		mutable std::mutex mtx_;
		double flt_mean_ = 0.0; // Mean value. Filtered!!!
		double flt_ss_ = 0.0; // Sum of squares. Filtered!!!
		double flt_amt_ = 0U; // Number of items processed. Filtered!!!
		size_t flt_calls_ = 0U; // Number of invoks processed. Filtered!!!
		VI_TM_TDIFF sum_ = 0U; // Sum of all measurements.
		size_t calls_ = 0U; // Number of calls to 'add()'.
		size_t amt_ = 0U; // Number of items for measurements.
#else
		// Simple statistics: no standard deviation, only the mean value.
		std::atomic<VI_TM_TDIFF> sum_ = 0U;
		std::atomic<size_t> calls_ = 0U;
		std::atomic<size_t> amt_ = 0U;
#endif
		inline void add(VI_TM_TDIFF val, size_t amt) noexcept;
		vi_tmMeasuringRAW_t get() const noexcept;
		void reset() noexcept;
	};

	using storage_t = std::unordered_map<std::string, measuring_t>;
} // namespace

struct vi_tmMeasuring_t: storage_t::value_type {/**/};
static_assert
	(	sizeof(vi_tmMeasuring_t) == sizeof(storage_t::value_type),
		"'vi_tmMeasuring_t' should simply be a synonym for 'storage_t::value_type'."
	);

struct vi_tmJournal_t
{
private:
	static constexpr auto MAX_LOAD_FACTOR = 0.7F;
	static constexpr size_t DEFAULT_STORAGE_CAPACITY = 64U;
	std::mutex storage_guard_;
	storage_t storage_;
public:
	static auto& from_handle(VI_TM_HJOUR journal); // Get the journal from the handle or return the global journal.
	explicit vi_tmJournal_t();
	int init();
	auto& at(const char *name); // Get a reference to the measurement by name, creating it if it does not exist.
	int for_each_measurement(vi_tmMeasEnumCallback_t fn, void *data); // Calls the function fn for each measurement in the journal, while this function returns 0. Returns the return code of the function fn if it returned a nonzero value, or 0 if all measurements were processed.
	void reset(const char *name = nullptr); // Resets a specific measurement by name or all measurements if name is nullptr.
};

void measuring_t::add(VI_TM_TDIFF v, size_t n) noexcept
{	if(verify(!!n))
	{
#ifdef VI_TM_STAT_USE_WELFORD
		static constexpr double K2 = 6.25; // 2.5^2 Threshold for outliers.
		const auto v_f = static_cast<double>(v);
		const auto n_f = static_cast<double>(n);

		std::lock_guard lg(mtx_);
		calls_++;
		sum_ += v;
		amt_ += n;

		const auto flt_amt_f = flt_amt_;
		const auto deviation = v_f / n_f - flt_mean_; // Difference from the mean value.
		const auto sum_square_dev = deviation * deviation * flt_amt_f;
		if 
		(	flt_amt_ <= 2.0 || // If we have less than 3 measurements, we cannot calculate the standard deviation.
			flt_ss_ <= 1.0 || // A pair of zero initial measurements will block the addition of other.
			deviation < 0.0 || // The minimum value is usually closest to the true value.
			sum_square_dev < K2 * flt_ss_ // Avoids outliers.
		)
		{	flt_amt_ += n_f;
			const auto rev_total_f = 1.0 / flt_amt_;
			flt_mean_ = (flt_mean_ * flt_amt_f + v_f) * rev_total_f;
			flt_ss_ += sum_square_dev * n_f * rev_total_f;
			flt_calls_++; // Increment the number of invocations only if the value was added to the statistics.
		}
#else
		calls_.fetch_add(1, std::memory_order_relaxed);
		sum_.fetch_add(v, std::memory_order_relaxed);
		amt_.fetch_add(n, std::memory_order_relaxed);
#endif
	}
}

vi_tmMeasuringRAW_t measuring_t::get() const noexcept
{	vi_tmMeasuringRAW_t result;
#ifdef VI_TM_STAT_USE_WELFORD
	std::lock_guard lg(mtx_);
	result.flt_mean_ = flt_mean_;
	result.flt_ss_ = flt_ss_;
	result.flt_amt_ = static_cast<std::size_t>(flt_amt_);
	result.flt_calls_ = flt_calls_;
#endif
	result.sum_ = sum_;
	result.amt_ = amt_;
	result.calls_ = calls_;
	return result;
}

void measuring_t::reset() noexcept
{
#ifdef VI_TM_STAT_USE_WELFORD
	std::lock_guard lg(mtx_);
	flt_mean_ = flt_ss_ = 0.0;
	flt_amt_ = 0U;
#endif
	sum_ = 0U;
	amt_ = calls_ = 0U;
}

inline auto& vi_tmJournal_t::from_handle(VI_TM_HJOUR journal)
{	static vi_tmJournal_t global;
	return journal ? *journal : global;
}

vi_tmJournal_t::vi_tmJournal_t()
{	storage_.max_load_factor(MAX_LOAD_FACTOR);
	storage_.reserve(DEFAULT_STORAGE_CAPACITY);
}

int vi_tmJournal_t::init()
{	return 0;
}

inline auto& vi_tmJournal_t::at(const char *name)
{	std::lock_guard lock{ storage_guard_ };
	return *storage_.try_emplace(name).first;
}

int vi_tmJournal_t::for_each_measurement(vi_tmMeasEnumCallback_t fn, void *data)
{	std::lock_guard lock{ storage_guard_ };
	for (auto &it : storage_)
	{	assert(it.second.amt_ >= it.second.calls_);
		if (!it.first.empty())
		{	if (const auto interrupt = std::invoke(fn, static_cast<VI_TM_HMEAS>(&it), data))
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
// This code is only compiled in debug mode to test certain library functionality.
namespace
{
	const auto nanotest = []
		{
			static constexpr VI_TM_TDIFF samples_simple[] = { 34, 32, 36 }; // Samples that will be added one at a time.
			static constexpr auto M = 2;
			static constexpr VI_TM_TDIFF samples_multiple[] = { 34, }; // Samples that will be added M times at once.
			static constexpr VI_TM_TDIFF samples_exclude[] = { 1000 }; // Samples that will be excluded from the statistics.

			static constexpr auto exp_flt_cnt = std::size(samples_simple) + M * std::size(samples_multiple); // The total number of samples that will be counted.
			static const auto exp_flt_mean = 
				(	std::accumulate(std::cbegin(samples_simple), std::cend(samples_simple), 0.0) +
					M * std::accumulate(std::cbegin(samples_multiple), std::cend(samples_multiple), 0.0)
				) / static_cast<double>(exp_flt_cnt); // The mean value of the samples that will be counted.
#	ifdef VI_TM_STAT_USE_WELFORD
			const auto exp_flt_stddev = [] // The standard deviation of the samples that will be counted.
				{	const auto sum_squared_deviations =
					std::accumulate
					(	std::cbegin(samples_simple),
						std::cend(samples_simple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<double>(v) - exp_flt_mean; return std::fma(d, d, i); }
					) +
					M * std::accumulate
					(	std::cbegin(samples_multiple),
						std::cend(samples_multiple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<double>(v) - exp_flt_mean; return std::fma(d, d, i); }
					);
					return std::sqrt(sum_squared_deviations / static_cast<double>(exp_flt_cnt - 1));
				}();
#	endif
			static constexpr char NAME[] = "dummy"; // The name of the measurement.

			const char *name = nullptr; // Name of the measurement to be filled in.
			vi_tmMeasuringRAW_t md; // Measurement data to be filled in.
			std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> journal{ vi_tmJournalCreate(), vi_tmJournalClose }; // Journal for measurements, automatically closed on destruction.
			{	const auto m = vi_tmMeasuring(journal.get(), NAME); // Create a measurement 'NAME'.
				for (auto x : samples_simple) // Add simple samples one at a time.
				{	vi_tmMeasuringRepl(m, x);
				}
				for (auto x : samples_multiple) // Add multiple samples M times at once.
				{	vi_tmMeasuringRepl(m, M * x, M);
				}
#	ifdef VI_TM_STAT_USE_WELFORD
				for (auto x : samples_exclude) // Add samples that will be excluded from the statistics.
				{	vi_tmMeasuringRepl(m, x, 1);
				}
#	endif
				vi_tmMeasuringGet(m, &name, &md); // Get the measurement data and name.
			}

			assert(name && std::strlen(name) + 1 == std::size(NAME) && 0 == std::strcmp(name, NAME));
#	ifdef VI_TM_STAT_USE_WELFORD
			assert(md.calls_ == std::size(samples_simple) + std::size(samples_multiple) + std::size(samples_exclude));
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple) + std::size(samples_exclude));
			assert(md.flt_amt_ == exp_flt_cnt);
			assert(std::abs(md.flt_mean_ - exp_flt_mean) / exp_flt_mean < 1e-12);
			const auto s = std::sqrt(md.flt_ss_ / static_cast<double>(md.flt_amt_ - 1U));
			assert(std::abs(s - exp_flt_stddev) / exp_flt_stddev < 1e-12);
#	else
			assert(md.calls_ == std::size(samples_simple) + std::size(samples_multiple));
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple));
			assert(std::abs(static_cast<double>(md.sum_) / md.amt_ - exp_flt_mean) < 1e-12);
#	endif
			return 0;
		}();
}
#endif // #ifndef NDEBUG

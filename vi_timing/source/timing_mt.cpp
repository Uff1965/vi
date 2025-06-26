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

#include "../vi_timing_c.h"
#include "build_number_maker.h" // For build number generation.
#include "welford_alg.h" // For Welford's algorithm implementation.

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
	inline bool verify(bool b) { assert(b && "Verify failed!"); return b; }

	struct measuring_t
	{
		// Welford’s online algorithm for computing variance and filtering of outliers.
		mutable std::mutex mtx_;
		welford_t welford_; // Filtered statistics using Welford's algorithm.
		VI_TM_TDIFF sum_ = 0U; // Sum of all measurements.
		size_t calls_ = 0U; // Number of calls to 'add()'.
		size_t amt_ = 0U; // Number of items for measurements.

		inline void add(VI_TM_TDIFF val, size_t amt) noexcept;
		vi_tmMeasuringRAW_t get() const noexcept;
		void reset() noexcept;
	};

	using storage_t = std::unordered_map<std::string, measuring_t>;
} // namespace

struct vi_tmMeasuring_t: storage_t::value_type {/**/};
static_assert
	(	sizeof(vi_tmMeasuring_t) == sizeof(storage_t::value_type) && alignof(vi_tmMeasuring_t) == alignof(storage_t::value_type),
		"'vi_tmMeasuring_t' should simply be a synonym for 'storage_t::value_type'."
	);

struct vi_tmJournal_t
{
private:
	static constexpr auto MAX_LOAD_FACTOR = 0.7F;
	static constexpr size_t DEFAULT_STORAGE_CAPACITY = 64U;
	std::mutex storage_guard_;
	storage_t storage_;
	bool need_report_ = false;
public:
	static auto& from_handle(VI_TM_HJOUR journal); // Get the journal from the handle or return the global journal.
	explicit vi_tmJournal_t(bool need_report = false);
	~vi_tmJournal_t();
	int init();
	auto& try_emplace(const char *name); // Get a reference to the measurement by name, creating it if it does not exist.
	int for_each_measurement(vi_tmMeasEnumCallback_t fn, void *data); // Calls the function fn for each measurement in the journal, while this function returns 0. Returns the return code of the function fn if it returned a nonzero value, or 0 if all measurements were processed.
	void clear();
};

void measuring_t::add(VI_TM_TDIFF v, size_t n) noexcept
{	
	if (!verify(!!n))
	{	return;
	}

	const auto v_f = static_cast<double>(v);
	const auto n_f = static_cast<double>(n);

	{
		std::lock_guard lg(mtx_);
		++calls_;
		sum_ += v;
		amt_ += n;
		welford_.update(v_f, n_f);
	}
}

vi_tmMeasuringRAW_t measuring_t::get() const noexcept
{	vi_tmMeasuringRAW_t result;
	std::lock_guard lg(mtx_);
	result.flt_mean_ = welford_.mean_;
	result.flt_ss_ = welford_.ss_;
	result.flt_amt_ = static_cast<std::size_t>(welford_.amt_);
	result.flt_calls_ = welford_.calls_;
	result.sum_ = sum_;
	result.amt_ = amt_;
	result.calls_ = calls_;
	return result;
}

void measuring_t::reset() noexcept
{
	std::lock_guard lg(mtx_);
	welford_.mean_ = welford_.ss_ = welford_.amt_ = 0.0;
	welford_.calls_ = 0U;
	sum_ = 0U;
	amt_ = calls_ = 0U;
}

inline auto& vi_tmJournal_t::from_handle(VI_TM_HJOUR journal)
{	static vi_tmJournal_t global{true};
	assert(journal);
	return VI_TM_HGLOBAL == journal ? global : *journal;
}

vi_tmJournal_t::vi_tmJournal_t(bool need_report)
: need_report_(need_report)
{	storage_.max_load_factor(MAX_LOAD_FACTOR);
	storage_.reserve(DEFAULT_STORAGE_CAPACITY);
}

vi_tmJournal_t::~vi_tmJournal_t()
{	if (need_report_)
	{	vi_tmReport(this, vi_tmShowResolution | vi_tmShowDuration | vi_tmSortByName);
	}
}

int vi_tmJournal_t::init()
{	auto &journal = vi_tmJournal_t::from_handle(VI_TM_HGLOBAL);
	assert(journal.storage_.empty() && "The global journal should not be empty at initialization.");
	// Initialize the global journal here.
	((void)journal);
	return 0;
}

inline auto& vi_tmJournal_t::try_emplace(const char *name)
{
	std::lock_guard lock{ storage_guard_ };
	return *storage_.try_emplace(name).first;
}

int vi_tmJournal_t::for_each_measurement(vi_tmMeasEnumCallback_t fn, void *data)
{	std::lock_guard lock{ storage_guard_ };
	need_report_ = false; // No need to report. The user probebly make a report himself.
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

void vi_tmJournal_t::clear()
{	std::lock_guard lock{ storage_guard_ };
	storage_.clear();
}

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
int VI_TM_CALL vi_tmInit()
{	return vi_tmJournal_t::from_handle(VI_TM_HGLOBAL).init();
}

void VI_TM_CALL vi_tmFinit(void)
{	vi_tmJournal_t::from_handle(VI_TM_HGLOBAL).clear();
}

VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate(unsigned /*flags*/, void */*reserved*/)
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

void VI_TM_CALL vi_tmJournalReset(VI_TM_HJOUR journal) noexcept
{	vi_tmMeasuringEnumerate(journal, [](VI_TM_HMEAS m, void*) { vi_tmMeasuringReset(m); return 0; }, nullptr);
}

int VI_TM_CALL vi_tmMeasuringEnumerate(VI_TM_HJOUR journal, vi_tmMeasEnumCallback_t fn, void *data)
{	return vi_tmJournal_t::from_handle(journal).for_each_measurement(fn, data);
}

VI_TM_HMEAS VI_TM_CALL vi_tmMeasuring(VI_TM_HJOUR journal, const char *name)
{	return static_cast<VI_TM_HMEAS>(&vi_tmJournal_t::from_handle(journal).try_emplace(name));
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
				for (auto x : samples_exclude) // Add samples that will be excluded from the statistics.
				{	vi_tmMeasuringRepl(m, x, 1);
				}
				vi_tmMeasuringGet(m, &name, &md); // Get the measurement data and name.
			}

			assert(name && std::strlen(name) + 1 == std::size(NAME) && 0 == std::strcmp(name, NAME));
			assert(md.calls_ == std::size(samples_simple) + std::size(samples_multiple) + std::size(samples_exclude));
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple) + std::size(samples_exclude));
			assert(md.flt_amt_ == exp_flt_cnt);
			assert(std::abs(md.flt_mean_ - exp_flt_mean) / exp_flt_mean < 1e-12);
			const auto s = std::sqrt(md.flt_ss_ / static_cast<double>(md.flt_amt_ - 1U));
			assert(std::abs(s - exp_flt_stddev) / exp_flt_stddev < 1e-12);
			return 0;
		}();
}
#endif // #ifndef NDEBUG

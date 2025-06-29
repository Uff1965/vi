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

#include <cassert> // assert()
#include <cmath> // std::sqrt
#include <cstdint> // std::uint64_t, std::size_t
#include <functional> // std::invoke
#include <memory> // std::unique_ptr
#include <numeric> // std::accumulate
#include <string> // std::string
#include <unordered_map> // unordered_map: "does not invalidate pointers or references to elements".

#ifdef VI_TM_THREADSAFE
#	ifdef __STDC_NO_ATOMICS__
		//	At the moment Atomics are available in Visual Studio 2022 with the /experimental:c11atomics flag.
		//	"we left out support for some C11 optional features such as atomics" [Microsoft
		//	https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#		error "Atomic objects and the atomic operation library are not supported."
#	endif

#	include <atomic> // std::atomic
#	include <mutex> // std::mutex, std::lock_guard
#	include <thread> // std::this_thread::yield()

#	if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#		if defined(_MSC_VER)
#			include <intrin.h>
#			define cpu_relax() _mm_pause()
#		else
#			include <immintrin.h>
#			define cpu_relax() __builtin_ia32_pause()
#		endif
#	elif defined(__aarch64__) || defined(__arm__)
#		define cpu_relax() __asm__ __volatile__("yield") // For ARM architectures: use yield hint
#	else
#		define cpu_relax() std::this_thread::yield() // Fallback
#	endif

namespace
{
	class mutex_for_short_captures_t
	{	std::atomic_flag locked_ = ATOMIC_FLAG_INIT;
	public:
		void lock() noexcept
		{	while (locked_.test_and_set(std::memory_order_acquire))
			{	cpu_relax(); // Fast spin
			}
		}
		void unlock() noexcept { locked_.clear(std::memory_order_release); }
		bool try_lock() noexcept { return !locked_.test_and_set(std::memory_order_acquire); }
	};

	using mutex = mutex_for_short_captures_t;
}

#	define VI_THREADSAFE_ONLY(t) t
#else
#	define VI_THREADSAFE_ONLY(t)
#endif

static_assert(std::is_standard_layout_v<vi_tmMeasurementStats_t>);

namespace
{
	inline bool verify(bool b) { assert(b && "Verify failed!"); return b; }

	using fp_limits_t = std::numeric_limits<VI_TM_FP>;

#ifndef NDEBUG
	bool check_invariant(const vi_tmMeasurementStats_t& src)
	{	if(!verify(src.amt_ >= src.calls_)) return false;
		if (src.calls_ == 0U)
		{	if(!verify(src.amt_ == 0U)) return false;
			if(!verify(src.sum_ == 0U)) return false;
		}

	#ifdef VI_TM_STAT_USE_WELFORD
		if (!verify(src.calls_ >= src.flt_calls_)) return false;
		if (!verify(VI_TM_FP(src.amt_) >= src.flt_amt_)) return false;
		if (!verify(src.flt_amt_ >= VI_TM_FP(src.flt_calls_))) return false;
		if (VI_TM_FP _; !verify(std::modf(src.flt_amt_, &_) == VI_TM_FP(0))) return false;
		if (!verify(src.flt_mean_ >= 0.0)) return false;
		if (!verify(src.flt_ss_ >= 0.0)) return false;

		if (src.flt_amt_ == 0.0)
		{	if (!verify(src.flt_mean_ == 0.0)) return false;
			if (!verify(src.flt_ss_ == 0.0)) return false;
			if (!verify(src.min_ == fp_limits_t::infinity())) return false;
			if (!verify(src.max_ == -fp_limits_t::infinity())) return false;
		}
		else if (src.flt_amt_ == 1.0)
		{	if (!verify(src.flt_calls_ == 1U)) return false;
			if (!verify(src.min_ == src.max_)) return false;
			if (!verify(std::abs(src.min_ - src.flt_mean_) / src.flt_mean_ < fp_limits_t::epsilon())) return false;
		}
		else
		{	if (!verify(src.flt_calls_ >= 1U)) return false;
			if (!verify(std::nextafter(src.min_, fp_limits_t::lowest()) <= src.flt_mean_)) return false;
			if (!verify(std::nextafter(src.max_, fp_limits_t::max()) >= src.flt_mean_)) return false;
		}
	#endif
		return true;
	}
#endif

	class measuring_t: public vi_tmMeasurementStats_t
	{	VI_THREADSAFE_ONLY(mutable mutex mtx_);
	public:
		measuring_t() noexcept
		{	vi_tmMeasurementStatsReset(this);
		}
		void add(VI_TM_TDIFF val, VI_TM_SIZE amt) noexcept;
		void merge(const vi_tmMeasurementStats_t &src) noexcept;
		vi_tmMeasurementStats_t get() const noexcept;
		void reset() noexcept
		{	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
			vi_tmMeasurementStatsReset(this);
		}
	};

	using storage_t = std::unordered_map<std::string, measuring_t>;
} // namespace

struct vi_tmMeasurement_t: storage_t::value_type {/**/};
static_assert
	(	sizeof(vi_tmMeasurement_t) == sizeof(storage_t::value_type) && alignof(vi_tmMeasurement_t) == alignof(storage_t::value_type),
		"'vi_tmMeasurement_t' should simply be a synonym for 'storage_t::value_type'."
	);

struct vi_tmMeasurementsJournal_t
{
private:
	static constexpr auto MAX_LOAD_FACTOR = 0.7F;
	static constexpr size_t DEFAULT_STORAGE_CAPACITY = 64U;
	VI_THREADSAFE_ONLY(mutex storage_guard_);
	storage_t storage_;
	bool need_report_ = false;
public:
	static auto& from_handle(VI_TM_HJOUR journal); // Get the journal from the handle or return the global journal.
	explicit vi_tmMeasurementsJournal_t(bool need_report = false);
	~vi_tmMeasurementsJournal_t();
	int init();
	auto& try_emplace(const char *name); // Get a reference to the measurement by name, creating it if it does not exist.
	int for_each_measurement(vi_tmMeasEnumCb_t fn, void *data); // Calls the function fn for each measurement in the journal, while this function returns 0. Returns the return code of the function fn if it returned a nonzero value, or 0 if all measurements were processed.
	void clear();
};

void VI_TM_CALL vi_tmMeasurementStatsReset(vi_tmMeasurementStats_t *m) noexcept
{	m->calls_ = 0U;
	m->amt_ = 0U;
	m->sum_ = 0U;
#ifdef VI_TM_STAT_USE_WELFORD
	m->flt_calls_ = 0U;
	m->flt_amt_ = VI_TM_FP(0);
	m->flt_mean_ = VI_TM_FP(0);
	m->flt_ss_ = VI_TM_FP(0);
	m->min_ = fp_limits_t::infinity();
	m->max_ = -fp_limits_t::infinity();
#endif
	assert(check_invariant(*m));
}

void VI_TM_CALL vi_tmMeasurementStatsRepl(vi_tmMeasurementStats_t *meas, VI_TM_TDIFF dur, VI_TM_SIZE amt) noexcept
{	assert(meas);
	assert(check_invariant(*meas));

	if (!verify(!!amt))
	{	return;
	}

	++meas->calls_;
	meas->amt_ += amt;
	meas->sum_ += dur;

#ifdef VI_TM_STAT_USE_WELFORD
	constexpr VI_TM_FP K2 = 6.25; // 2.5^2 Threshold for outliers.
	const auto v_f = static_cast<VI_TM_FP>(dur);
	const auto n_f = static_cast<VI_TM_FP>(amt);

	const auto a_f = v_f / n_f;
	if (meas->min_ > a_f) { meas->min_ = a_f; }
	if (meas->max_ < a_f) { meas->max_ = a_f; }

	const auto deviation = v_f / n_f - meas->flt_mean_; // Difference from the mean value.
	if
	(	const auto sum_square_dev = deviation * deviation * meas->flt_amt_;
		meas->flt_amt_ <= 2.0 || // If we have less than 3 measurements, we cannot calculate the standard deviation.
		meas->flt_ss_ <= 1.0 || // A pair of zero initial measurements will block the addition of other.
		deviation < 0.0 || // The minimum value is usually closest to the true value.
		sum_square_dev < K2 * meas->flt_ss_ // Avoids outliers.
	)
	{	const auto flt_amt = meas->flt_amt_;
		meas->flt_amt_ += n_f;
		const auto rev_total = VI_TM_FP(1.0) / meas->flt_amt_;
		meas->flt_mean_ = std::fma(meas->flt_mean_, flt_amt, v_f) * rev_total;
		meas->flt_ss_ = std::fma(sum_square_dev, n_f * rev_total, meas->flt_ss_);
		meas->flt_calls_++; // Increment the number of invocations only if the value was added to the statistics.
	}
#endif
	assert(check_invariant(*meas));
}

inline void measuring_t::add(VI_TM_TDIFF v, VI_TM_SIZE n) noexcept
{	if (!verify(!!n))
	{	return;
	}

	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmMeasurementStatsRepl(this, v, n);
}

inline void measuring_t::merge(const vi_tmMeasurementStats_t &src) noexcept
{	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmMeasurementStatsMerge(this, &src);
}

void VI_TM_CALL vi_tmMeasurementStatsMerge(vi_tmMeasurementStats_t *dst, const vi_tmMeasurementStats_t *src) VI_NOEXCEPT
{	if(!verify(!!dst) || !verify(!!src) || src->amt_ == 0)
	{	return;
	}

	assert(check_invariant(*dst));
	assert(check_invariant(*src));

	dst->calls_ += src->calls_;
	dst->amt_ += src->amt_;
	dst->sum_ += src->sum_;

#ifdef VI_TM_STAT_USE_WELFORD
	if (src->flt_amt_ > VI_TM_FP(0))
	{	const auto rev_amt = VI_TM_FP(1) / (dst->flt_amt_ + src->flt_amt_);
		const auto diff_mean = src->flt_mean_ - dst->flt_mean_;
		dst->flt_ss_ = std::fma(dst->flt_amt_ * src->flt_amt_ * rev_amt, diff_mean * diff_mean, (dst->flt_ss_ + src->flt_ss_));
		dst->flt_mean_ = std::fma(dst->flt_mean_, dst->flt_amt_, src->flt_mean_ * src->flt_amt_) * rev_amt;
		dst->flt_amt_ += src->flt_amt_;
		dst->flt_calls_ += src->flt_calls_;
	}
#endif
	assert(check_invariant(*dst));
}

inline vi_tmMeasurementStats_t measuring_t::get() const noexcept
{	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	return *this;
}

inline auto& vi_tmMeasurementsJournal_t::from_handle(VI_TM_HJOUR journal)
{	static vi_tmMeasurementsJournal_t global{true};
	assert(journal);
	return VI_TM_HGLOBAL == journal ? global : *journal;
}

vi_tmMeasurementsJournal_t::vi_tmMeasurementsJournal_t(bool need_report)
	: need_report_(need_report)
{	storage_.max_load_factor(MAX_LOAD_FACTOR);
	storage_.reserve(DEFAULT_STORAGE_CAPACITY);
}

vi_tmMeasurementsJournal_t::~vi_tmMeasurementsJournal_t()
{	if (need_report_)
	{	vi_tmReport(this, vi_tmShowResolution | vi_tmShowDuration | vi_tmSortByName);
	}
}

int vi_tmMeasurementsJournal_t::init()
{	auto &journal = vi_tmMeasurementsJournal_t::from_handle(VI_TM_HGLOBAL);
	assert(journal.storage_.empty() && "The global journal should not be empty at initialization.");
	// Initialize the global journal here.
	((void)journal);
	return 0;
}

inline auto& vi_tmMeasurementsJournal_t::try_emplace(const char *name)
{
	VI_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
	return *storage_.try_emplace(name).first;
}

int vi_tmMeasurementsJournal_t::for_each_measurement(vi_tmMeasEnumCb_t fn, void *data)
{	VI_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
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

void vi_tmMeasurementsJournal_t::clear()
{	VI_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
	storage_.clear();
}

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
int VI_TM_CALL vi_tmInit()
{	return vi_tmMeasurementsJournal_t::from_handle(VI_TM_HGLOBAL).init();
}

void VI_TM_CALL vi_tmFinit(void)
{	vi_tmMeasurementsJournal_t::from_handle(VI_TM_HGLOBAL).clear();
}

VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate(unsigned /*flags*/, void */*reserved*/)
{	try
	{	return new vi_tmMeasurementsJournal_t{};
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
{	vi_tmMeasurementEnumerate(journal, [](VI_TM_HMEAS m, void*) { vi_tmMeasurementReset(m); return 0; }, nullptr);
}

int VI_TM_CALL vi_tmMeasurementEnumerate(VI_TM_HJOUR journal, vi_tmMeasEnumCb_t fn, void *data)
{	return vi_tmMeasurementsJournal_t::from_handle(journal).for_each_measurement(fn, data);
}

VI_TM_HMEAS VI_TM_CALL vi_tmMeasurement(VI_TM_HJOUR journal, const char *name)
{	return static_cast<VI_TM_HMEAS>(&vi_tmMeasurementsJournal_t::from_handle(journal).try_emplace(name));
}

void VI_TM_CALL vi_tmMeasurementRepl(VI_TM_HMEAS meas, VI_TM_TDIFF tick_diff, VI_TM_SIZE amount) noexcept
{	if (verify(meas)) { meas->second.add(tick_diff, amount); }
}

void VI_TM_CALL vi_tmMeasurementMerge(VI_TM_HMEAS meas, const vi_tmMeasurementStats_t *src) noexcept
{	if (verify(meas)) { meas->second.merge(*src); }
}

void VI_TM_CALL vi_tmMeasurementGet(VI_TM_HMEAS meas, const char* *name, vi_tmMeasurementStats_t *data)
{	if (verify(meas))
	{	if (name) { *name = meas->first.c_str(); }
		if (data) { *data = meas->second.get(); }
	}
}

void VI_TM_CALL vi_tmMeasurementReset(VI_TM_HMEAS meas)
{	if (verify(meas)) { meas->second.reset(); }
}
//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifndef NDEBUG
// This code is only compiled in debug mode to test certain library functionality.
namespace
{
	constexpr auto DBG_EPS()
	{	return fp_limits_t::epsilon();
	};

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
				) / static_cast<VI_TM_FP>(exp_flt_cnt); // The mean value of the samples that will be counted.
#	ifdef VI_TM_STAT_USE_WELFORD
			const auto exp_flt_stddev = [] // The standard deviation of the samples that will be counted.
				{	const auto sum_squared_deviations =
					std::accumulate
					(	std::cbegin(samples_simple),
						std::cend(samples_simple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - exp_flt_mean; return std::fma(d, d, i); }
					) +
					M * std::accumulate
					(	std::cbegin(samples_multiple),
						std::cend(samples_multiple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - exp_flt_mean; return std::fma(d, d, i); }
					);
					return std::sqrt(sum_squared_deviations / static_cast<VI_TM_FP>(exp_flt_cnt - 1));
				}();
#	endif
			static constexpr char NAME[] = "dummy"; // The name of the measurement.

			const char *name = nullptr; // Name of the measurement to be filled in.
			vi_tmMeasurementStats_t md; // Measurement data to be filled in.
			std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> journal{ vi_tmJournalCreate(), vi_tmJournalClose }; // Journal for measurements, automatically closed on destruction.
			{	const auto m = vi_tmMeasurement(journal.get(), NAME); // Create a measurement 'NAME'.
				for (auto x : samples_simple) // Add simple samples one at a time.
				{	vi_tmMeasurementRepl(m, x);
				}

				vi_tmMeasurementStatsReset(&md);
				for (auto x : samples_multiple) // Add multiple samples M times at once.
				{	vi_tmMeasurementStatsRepl(&md, M * x, M);
				}

				vi_tmMeasurementMerge(m, &md); // Merge the statistics into the measurement.
				vi_tmMeasurementStatsReset(&md);

#	ifdef VI_TM_STAT_USE_WELFORD
				for (auto x : samples_exclude) // Add samples that will be excluded from the statistics.
				{	vi_tmMeasurementRepl(m, x, 1);
				}
#	endif
				vi_tmMeasurementGet(m, &name, &md); // Get the measurement data and name.
			}

			assert(name && std::strlen(name) + 1 == std::size(NAME) && 0 == std::strcmp(name, NAME));
#	ifdef VI_TM_STAT_USE_WELFORD
			assert(md.calls_ == std::size(samples_simple) + std::size(samples_multiple) + std::size(samples_exclude));
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple) + std::size(samples_exclude));
			assert(md.flt_amt_ == exp_flt_cnt);
			assert(std::abs(md.flt_mean_ - exp_flt_mean) / exp_flt_mean < DBG_EPS());
			const auto s = std::sqrt(md.flt_ss_ / (md.flt_amt_ - VI_TM_FP(1)));
			assert(std::abs(s - exp_flt_stddev) / exp_flt_stddev < DBG_EPS());
#	else
			assert(md.calls_ == std::size(samples_simple) + std::size(samples_multiple));
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple));
			assert(std::abs(static_cast<VI_TM_FP>(md.sum_) / md.amt_ - exp_flt_mean) < DBG_EPS());
#	endif
			return 0;
		}();
}
#endif // #ifndef NDEBUG

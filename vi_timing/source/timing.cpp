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
#include "misc.h"

#include <cassert> // assert()
#include <chrono> // std::chrono::milliseconds
#include <cmath> // std::sqrt
#include <cstdint> // std::uint64_t, std::size_t
#include <functional> // std::invoke
#include <memory> // std::unique_ptr
#include <mutex> // std::mutex, std::lock_guard
#include <numeric> // std::accumulate
#include <string> // std::string
#include <unordered_map> // unordered_map: "does not invalidate pointers or references to elements".
#include <new>

#ifdef VI_TM_THREADSAFE
#	ifdef __STDC_NO_ATOMICS__
		//	At the moment Atomics are available in Visual Studio 2022 with the /experimental:c11atomics flag.
		//	"we left out support for some C11 optional features such as atomics" [Microsoft
		//	https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#		error "Atomic objects and the atomic operation library are not supported."
#	endif

#	include <atomic> // std::atomic
#	include <thread> // std::this_thread::yield()

// define CPU_RELAX for adaptive_mutex_t.
#	if defined(_MSC_VER)
#		include <intrin.h>
#		define CPU_RELAX() _mm_pause()
#	elif defined(__i386__) || defined(__x86_64__)
#		include <immintrin.h>
#		define CPU_RELAX() _mm_pause() // __builtin_ia32_pause()
#	elif defined(__arm__) || defined(__aarch64__)
#		define CPU_RELAX() __asm__ __volatile__("yield") // For ARM architectures: use yield hint
#	else
#		define CPU_RELAX() std::this_thread::yield() // Fallback
#	endif

namespace
{
	// A mutex optimized for short captures, using spin-waiting and yielding to reduce contention.
	// This mutex is designed to be used in scenarios where the lock is held for a very short time,
	// minimizing the overhead of locking and unlocking.
	class adaptive_mutex_t
	{	std::atomic_flag locked_ = ATOMIC_FLAG_INIT;
	public:
		adaptive_mutex_t() noexcept = default;
		adaptive_mutex_t(const adaptive_mutex_t&) = delete;
		adaptive_mutex_t& operator=(const adaptive_mutex_t&) = delete;

		void lock() noexcept
		{	constexpr unsigned SPIN_LIMIT = 50;
			constexpr unsigned YIELD_LIMIT = 100;

			for(unsigned spins = 0; locked_.test_and_set(std::memory_order_acquire); ++spins)
			{	if (spins < SPIN_LIMIT)
				{	CPU_RELAX(); // Spin-wait with a CPU relaxation hint.
				}
				else if (spins < SPIN_LIMIT + YIELD_LIMIT)
				{	std::this_thread::yield(); // Yield the thread to allow other threads to run.
				}
				else
				{	const auto sleep_ms = 1U << std::min(spins - (SPIN_LIMIT + YIELD_LIMIT), 5U); // exponential back-off
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
				}
			}
		}
		void unlock() noexcept { locked_.clear(std::memory_order_release); }
		bool try_lock() noexcept { return !locked_.test_and_set(std::memory_order_acquire); }
	};
}

#	define VI_THREADSAFE_ONLY(t) t
#else
#	define VI_THREADSAFE_ONLY(t)
#endif

// Checking for FMA support by the compiler/platform
#if defined(__FMA__) || defined(__AVX2__) || defined(__AVX512F__)
    #define FMA(x, y, z) std::fma((x), (y), (z))
#else
    #define FMA(x, y, z) ((x) * (y) + (z))
#endif

namespace
{
	using fp_limits_t = std::numeric_limits<VI_TM_FP>;
	constexpr auto fp_ZERO = static_cast<VI_TM_FP>(0);
	constexpr auto fp_ONE = static_cast<VI_TM_FP>(1);
	constexpr auto fp_EPSILON = fp_limits_t::epsilon();

	class alignas(std::hardware_constructive_interference_size) meterage_t
	{	static_assert(std::is_standard_layout_v<vi_tmMeasurementStats_t>);
		vi_tmMeasurementStats_t stats_;
		VI_THREADSAFE_ONLY(mutable adaptive_mutex_t mtx_);
	public:
		meterage_t() noexcept
		{	vi_tmMeasurementStatsReset(&stats_);
		}
		void add(VI_TM_TDIFF val, VI_TM_SIZE amt) noexcept;
		void merge(const vi_tmMeasurementStats_t & VI_RESTRICT src) noexcept;
		vi_tmMeasurementStats_t get() const noexcept;
		void reset() noexcept;
#pragma warning(suppress: 4324)
	};

	using storage_t = std::unordered_map<std::string, meterage_t>;
}

// 'vi_tmMeasurement_t' is simply an alias for a measurement entry in the storage map.
// It inherits from 'storage_t::value_type', which is typically 'std::pair<const std::string, meterage_t>'.
struct vi_tmMeasurement_t: storage_t::value_type {/**/};
static_assert
	(	sizeof(vi_tmMeasurement_t) == sizeof(storage_t::value_type) &&
		alignof(vi_tmMeasurement_t) == alignof(storage_t::value_type),
		"'vi_tmMeasurement_t' should simply be a synonym for 'storage_t::value_type'."
	);

struct vi_tmMeasurementsJournal_t
{
private:
	static constexpr auto MAX_LOAD_FACTOR = 0.7F;
	static constexpr size_t DEFAULT_STORAGE_CAPACITY = 64U;
	static inline std::mutex global_mtx_;
	static inline std::size_t global_initialized_ = 0U;
	storage_t storage_;
	bool need_report_ = false;
	VI_THREADSAFE_ONLY(adaptive_mutex_t storage_guard_);
public:
	vi_tmMeasurementsJournal_t(const vi_tmMeasurementsJournal_t &) = delete;
	vi_tmMeasurementsJournal_t &operator=(const vi_tmMeasurementsJournal_t &) = delete;
	explicit vi_tmMeasurementsJournal_t(bool need_report = false);
	~vi_tmMeasurementsJournal_t();
	int init();
	int finit();
	auto& try_emplace(const char *name); // Get a reference to the measurement by name, creating it if it does not exist.
	int for_each_measurement(vi_tmMeasEnumCb_t fn, void *data); // Calls the function fn for each measurement in the journal, while this function returns 0. Returns the return code of the function fn if it returned a nonzero value, or 0 if all measurements were processed.
	void clear();
	// Global journal management functions.
	static int global_init(); // Initialize the global journal.
	static int global_finit();
	static auto& from_handle(VI_TM_HJOUR journal); // Get the journal from the handle or return the global journal.
};

inline void meterage_t::reset() noexcept
{	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmMeasurementStatsReset(&stats_);
}

inline void meterage_t::add(VI_TM_TDIFF v, VI_TM_SIZE n) noexcept
{	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmMeasurementStatsAdd(&stats_, v, n);
}

inline void meterage_t::merge(const vi_tmMeasurementStats_t & VI_RESTRICT src) noexcept
{	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmMeasurementStatsMerge(&stats_, &src);
}

inline vi_tmMeasurementStats_t meterage_t::get() const noexcept
{	VI_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	return stats_;
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

	if (this == &from_handle(VI_TM_HGLOBAL))
	{	std::lock_guard lg{ global_mtx_ };
		assert(0U == global_initialized_ && "The number of library initializations does not match the number of deinitializations!");
	}
}

int vi_tmMeasurementsJournal_t::init()
{	return VI_EXIT_SUCCESS;
}

int vi_tmMeasurementsJournal_t::finit()
{	clear(); // Clear the journal on finalization.
	return VI_EXIT_SUCCESS;
}

inline auto& vi_tmMeasurementsJournal_t::try_emplace(const char *name)
{	assert(name);
	VI_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
	return *storage_.try_emplace(name).first;
}

int vi_tmMeasurementsJournal_t::for_each_measurement(vi_tmMeasEnumCb_t fn, void *data)
{	VI_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
	need_report_ = false; // No need to report. The user probebly make a report himself.
	for (auto &it : storage_)
	{	if (!it.first.empty())
		{	if (const auto breaker = std::invoke(fn, static_cast<VI_TM_HMEAS>(&it), data))
			{	return breaker;
			}
		}
	}
	return 0;
}

void vi_tmMeasurementsJournal_t::clear()
{	VI_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
	storage_.clear();
}

int vi_tmMeasurementsJournal_t::global_init()
{	std::lock_guard lg{global_mtx_};

	if (global_initialized_++ == 0U)
	{	auto& global = from_handle(VI_TM_HGLOBAL);
		(void)misc::verify(VI_EXIT_SUCCESS == global.init());
	}
	return VI_EXIT_SUCCESS;
}

int vi_tmMeasurementsJournal_t::global_finit()
{	std::lock_guard lg{global_mtx_};

	if (misc::verify(0U != global_initialized_) && 0U == --global_initialized_)
	{	auto& global = from_handle(VI_TM_HGLOBAL);
		(void)misc::verify(VI_EXIT_SUCCESS == global.finit());
	}
	return VI_EXIT_SUCCESS;
}

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
int VI_TM_CALL vi_tmMeasurementStatsIsValid(const vi_tmMeasurementStats_t *meas) noexcept
{	if(!misc::verify(!!meas))
	{	return VI_EXIT_FAILURE;
	}

#ifdef VI_TM_STAT_USE_BASE
	if (!misc::verify((0U != meas->amt_) == (0U != meas->calls_))) return VI_EXIT_FAILURE; // amt_ and calls_ must be both zero or both non-zero.
	if (!misc::verify(meas->amt_ >= meas->calls_)) return VI_EXIT_FAILURE; // amt_ must be greater than or equal to calls_.
#endif

#ifdef VI_TM_STAT_USE_MINMAX
	if (meas->calls_ == 0U)
	{	if (!misc::verify(meas->min_ == fp_limits_t::infinity())) return VI_EXIT_FAILURE; // If calls_ is zero, min_ must be infinity.
		if (!misc::verify(meas->max_ == -fp_limits_t::infinity())) return VI_EXIT_FAILURE; // If calls_ is zero, max_ must be negative infinity. 
	}
	else
	{	if (!misc::verify(meas->min_ != fp_limits_t::infinity())) return VI_EXIT_FAILURE; // min_ must not be infinity.
		if (meas->calls_ == 1U)
		{	if (!misc::verify(meas->min_ == meas->max_)) return VI_EXIT_FAILURE; // If there is only one call, min_ and max_ must be equal.
		}
		else
		{	if (!misc::verify(meas->min_ <= meas->max_)) return VI_EXIT_FAILURE; // min_ must be less than or equal to max_.
		}
	}
#endif

#if defined(VI_TM_STAT_USE_BASE) && defined(VI_TM_STAT_USE_MINMAX)
	if (meas->calls_ == 1U && !misc::verify(static_cast<VI_TM_FP>(meas->sum_) == meas->min_ * meas->amt_)) return VI_EXIT_FAILURE; // If there is only one call, sum_ must equal min_ multiplied by amt_.
	if (meas->calls_ >= 1U && !misc::verify(static_cast<VI_TM_FP>(meas->sum_) >= meas->max_)) return VI_EXIT_FAILURE; // sum_ must be greater than or equal to max_.
#endif

#ifdef VI_TM_STAT_USE_WELFORD
	if (!misc::verify(meas->flt_calls_ <= meas->calls_)) return VI_EXIT_FAILURE; // flt_calls_ must be less than or equal to calls_.
	if (!misc::verify((fp_ZERO != meas->flt_amt_) == (0U != meas->flt_calls_))) return VI_EXIT_FAILURE; // flt_amt_ and flt_calls_ must be both zero or both non-zero.
	if (!misc::verify(meas->flt_amt_ >= static_cast<VI_TM_FP>(meas->flt_calls_))) return VI_EXIT_FAILURE; // flt_amt_ must be greater than or equal to flt_calls_.
	if (VI_TM_FP _; !misc::verify(std::modf(meas->flt_amt_, &_) == fp_ZERO)) return VI_EXIT_FAILURE; // flt_amt_ must be an integer value.
	if (!misc::verify(meas->flt_mean_ >= fp_ZERO)) return VI_EXIT_FAILURE; // flt_mean_ must be non-negative.
	if (!misc::verify(meas->flt_ss_ >= fp_ZERO)) return VI_EXIT_FAILURE; // flt_ss_ must be non-negative.

	if (meas->flt_amt_ == fp_ZERO)
	{	if (!misc::verify(meas->flt_mean_ == fp_ZERO)) return VI_EXIT_FAILURE; // If flt_amt_ is zero, flt_mean_ must also be zero.
		if (!misc::verify(meas->flt_ss_ == fp_ZERO)) return VI_EXIT_FAILURE; // If flt_amt_ is zero, flt_ss_ must also be zero.
	}
	else if (meas->flt_amt_ == fp_ONE)
	{	if (!misc::verify(meas->flt_ss_ == fp_ZERO)) return VI_EXIT_FAILURE;
	}
#endif

#if defined(VI_TM_STAT_USE_BASE) && defined(VI_TM_STAT_USE_WELFORD)
	if (!misc::verify(meas->flt_amt_ <= static_cast<VI_TM_FP>(meas->amt_))) return VI_EXIT_FAILURE; // flt_amt_ must be less than or equal to amt_.
#endif

#if defined(VI_TM_STAT_USE_MINMAX) && defined(VI_TM_STAT_USE_WELFORD)
	if (meas->flt_calls_ > 0U)
	{	if (!misc::verify((meas->min_ - meas->flt_mean_) / meas->flt_mean_ < fp_EPSILON)) return VI_EXIT_FAILURE;
		if (!misc::verify((meas->flt_mean_ - meas->max_) / meas->flt_mean_ < fp_EPSILON)) return VI_EXIT_FAILURE;
	}
#endif

	return VI_EXIT_SUCCESS;
}

void VI_TM_CALL vi_tmMeasurementStatsReset(vi_tmMeasurementStats_t *meas) noexcept
{	if (!misc::verify(!!meas))
	{	return;
	}

	meas->calls_ = 0U;
#ifdef VI_TM_STAT_USE_BASE
	meas->amt_ = 0U;
	meas->sum_ = 0U;
#endif
#ifdef VI_TM_STAT_USE_MINMAX
	meas->min_ = fp_limits_t::infinity();
	meas->max_ = -fp_limits_t::infinity();
#endif
#ifdef VI_TM_STAT_USE_WELFORD
	meas->flt_calls_ = 0U;
	meas->flt_amt_ = fp_ZERO;
	meas->flt_mean_ = fp_ZERO;
	meas->flt_ss_ = fp_ZERO;
#endif
	assert(VI_EXIT_SUCCESS == vi_tmMeasurementStatsIsValid(meas));
}

void VI_TM_CALL vi_tmMeasurementStatsAdd(vi_tmMeasurementStats_t *meas, VI_TM_TDIFF dur, VI_TM_SIZE amt) noexcept
{	(void)dur;
	if (!misc::verify(!!meas) || 0U == amt)
	{	return;
	}
	assert(VI_EXIT_SUCCESS == vi_tmMeasurementStatsIsValid(meas));

#if defined(VI_TM_STAT_USE_WELFORD) || defined(VI_TM_STAT_USE_MINMAX)
	const auto f_dur = static_cast<VI_TM_FP>(dur);
	const auto f_amt = static_cast<VI_TM_FP>(amt);
	const auto f_val = f_dur / f_amt;
#endif

	if (0U == meas->calls_++)
	{	// No complex calculations are required for the first (and possibly only) call.
#ifdef VI_TM_STAT_USE_BASE
		meas->amt_ = amt;
		meas->sum_ = dur;
#endif
#ifdef VI_TM_STAT_USE_MINMAX
		meas->min_ = f_val;
		meas->max_ = f_val;
#endif
#ifdef VI_TM_STAT_USE_WELFORD
		meas->flt_calls_ = 1U; // The first call cannot be filtered.
		meas->flt_amt_ = f_amt;
		meas->flt_mean_ = f_val; // The first value is the mean.
#endif
	}
	else
	{
#ifdef VI_TM_STAT_USE_BASE
		meas->amt_ += amt;
		meas->sum_ += dur;
#endif
#ifdef VI_TM_STAT_USE_MINMAX
		if (f_val < meas->min_) { meas->min_ = f_val; }
		if (f_val > meas->max_) { meas->max_ = f_val; }
#endif
#ifdef VI_TM_STAT_USE_WELFORD
		constexpr VI_TM_FP K = 2.5; // Threshold for outliers.
		if(	const auto deviation = f_val - meas->flt_mean_; // Difference from the mean value.
			dur <= 1U || // The measurable interval is probably smaller than the resolution of the clock.
			FMA(deviation * deviation, meas->flt_amt_, - K * K * meas->flt_ss_) < fp_ZERO || // Sigma clipping to avoids outliers.
			deviation < fp_ZERO || // The minimum value is usually closest to the true value. "deviation < .0" - for some reason slowly!!!
			meas->flt_calls_ <= 2U || // If we have less than 2 measurements, we cannot calculate the standard deviation.
			meas->flt_ss_ <= 1.0 // A pair of zero initial measurements will block the addition of other.
		)
		{	meas->flt_amt_ += f_amt;
			meas->flt_mean_ = FMA(deviation, f_amt / meas->flt_amt_, meas->flt_mean_);
			meas->flt_ss_ = FMA(deviation * (f_val - meas->flt_mean_), f_amt, meas->flt_ss_);
			meas->flt_calls_++;
		}
#endif
	}
	assert(VI_EXIT_SUCCESS == vi_tmMeasurementStatsIsValid(meas));
}

void VI_TM_CALL vi_tmMeasurementStatsMerge(vi_tmMeasurementStats_t* VI_RESTRICT dst, const vi_tmMeasurementStats_t* VI_RESTRICT src) noexcept
{	if(!misc::verify(!!dst && !!src && dst != src) && !!src->calls_)
	{	return;
	}

	assert(!!dst && !!src);
	assert(VI_EXIT_SUCCESS == vi_tmMeasurementStatsIsValid(dst));
	assert(VI_EXIT_SUCCESS == vi_tmMeasurementStatsIsValid(src));

	dst->calls_ += src->calls_;
#ifdef VI_TM_STAT_USE_BASE
	dst->amt_ += src->amt_;
	dst->sum_ += src->sum_;
#endif
#ifdef VI_TM_STAT_USE_MINMAX
	if(src->min_ < dst->min_)
	{	dst->min_ = src->min_;
	}
	if(src->max_ > dst->max_)
	{	dst->max_ = src->max_;
	}
#endif
#ifdef VI_TM_STAT_USE_WELFORD
	if (src->flt_amt_ > fp_ZERO)
	{	const auto new_amt_reverse = fp_ONE / (dst->flt_amt_ + src->flt_amt_);
		const auto diff_mean = src->flt_mean_ - dst->flt_mean_;
		dst->flt_mean_ = FMA(dst->flt_mean_, dst->flt_amt_, src->flt_mean_ * src->flt_amt_) * new_amt_reverse;
		dst->flt_ss_ = FMA(dst->flt_amt_ * diff_mean, src->flt_amt_ * diff_mean * new_amt_reverse, dst->flt_ss_ + src->flt_ss_);
		dst->flt_amt_ += src->flt_amt_;
		dst->flt_calls_ += src->flt_calls_;
	}
#endif
	assert(VI_EXIT_SUCCESS == vi_tmMeasurementStatsIsValid(dst));
}

int VI_TM_CALL vi_tmInit()
{	return vi_tmMeasurementsJournal_t::global_init();
}

void VI_TM_CALL vi_tmFinit(void)
{	vi_tmMeasurementsJournal_t::global_finit();
}

VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate(unsigned flags, void *reserved)
{	(void)reserved; // Reserved parameter, currently unused.
	try
	{	return new vi_tmMeasurementsJournal_t{0U != flags};
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
{	vi_tmMeasurementEnumerate(journal, [](VI_TM_HMEAS m, void *) { vi_tmMeasurementReset(m); return 0; }, nullptr);
}

int VI_TM_CALL vi_tmMeasurementEnumerate(VI_TM_HJOUR journal, vi_tmMeasEnumCb_t fn, void *data)
{	return vi_tmMeasurementsJournal_t::from_handle(journal).for_each_measurement(fn, data);
}

VI_TM_HMEAS VI_TM_CALL vi_tmMeasurement(VI_TM_HJOUR journal, const char *name)
{	return static_cast<VI_TM_HMEAS>(&vi_tmMeasurementsJournal_t::from_handle(journal).try_emplace(name));
}

void VI_TM_CALL vi_tmMeasurementAdd(VI_TM_HMEAS meas, VI_TM_TDIFF tick_diff, VI_TM_SIZE amount) noexcept
{	if (misc::verify(meas)) { meas->second.add(tick_diff, amount); }
}

void VI_TM_CALL vi_tmMeasurementMerge(VI_TM_HMEAS VI_RESTRICT meas, const vi_tmMeasurementStats_t * VI_RESTRICT src) noexcept
{	if (misc::verify(meas)) { meas->second.merge(*src); }
}

void VI_TM_CALL vi_tmMeasurementGet(VI_TM_HMEAS VI_RESTRICT meas, const char* *name, vi_tmMeasurementStats_t * VI_RESTRICT data)
{	if (misc::verify(meas))
	{	if (name) { *name = meas->first.c_str(); }
		if (data) { *data = meas->second.get(); }
	}
}

void VI_TM_CALL vi_tmMeasurementReset(VI_TM_HMEAS meas)
{	if (misc::verify(meas)) { meas->second.reset(); }
}
//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#if VI_TM_DEBUG
// This code is only compiled in debug mode to test certain library functionality.
namespace
{
	const auto nanotest1 = []
		{
			static constexpr char NAME[] = "dummy";
			const std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> journal
			{ vi_tmJournalCreate(),
				vi_tmJournalClose
			};
			const auto m = vi_tmMeasurement(journal.get(), NAME);

			{
				static constexpr VI_TM_TDIFF samples_simple[] =
				{	10010, 9981, 9948, 10030, 10053, 9929, 9894
				};

				for (auto x : samples_simple)
				{	vi_tmMeasurementAdd(m, x);
				}

				const char *name = nullptr;
				vi_tmMeasurementStats_t md;
				vi_tmMeasurementStatsReset(&md);
				vi_tmMeasurementGet(m, &name, &md);
				{	assert(!!name && 0 == strcmp(name, NAME));
					assert(md.calls_ == std::size(samples_simple));
#ifdef VI_TM_STAT_USE_BASE
					assert(md.amt_ == md.calls_);
					assert(md.sum_ == std::accumulate(std::begin(samples_simple), std::end(samples_simple), VI_TM_TDIFF(0U)));
#endif
#ifdef VI_TM_STAT_USE_WELFORD
					assert(md.flt_amt_ == static_cast<VI_TM_FP>(md.calls_));
					assert(md.flt_calls_ == md.calls_);
#endif
#ifdef VI_TM_STAT_USE_MINMAX
					assert(md.min_ == *std::min_element(std::begin(samples_simple), std::end(samples_simple)));
					assert(md.max_ == *std::max_element(std::begin(samples_simple), std::end(samples_simple)));
#endif
				}
				
#ifdef VI_TM_STAT_USE_WELFORD
				vi_tmMeasurementAdd(m, 10111); // It should be filtered out.
				{	vi_tmMeasurementStats_t tmp;
					vi_tmMeasurementStatsReset(&tmp);

					vi_tmMeasurementGet(m, &name, &tmp);

					assert(tmp.calls_ == md.calls_ + 1U);
					assert(tmp.calls_ == md.flt_calls_ + 1U);
				}
				vi_tmMeasurementGet(m, &name, &md);

				vi_tmMeasurementAdd(m, 10110); // It should not be filtered out.
				{	vi_tmMeasurementStats_t tmp;
					vi_tmMeasurementStatsReset(&tmp);

					vi_tmMeasurementGet(m, &name, &tmp);

					assert(tmp.calls_ == md.calls_ + 1U);
					assert(tmp.flt_calls_ == md.flt_calls_ + 1U);
					assert(tmp.flt_amt_ == md.flt_amt_ + fp_ONE);
				}
				vi_tmMeasurementGet(m, &name, &md);
#endif
			}

			return 0;
		}();

	const auto nanotest2 = []
		{
			static constexpr VI_TM_TDIFF samples_simple[] =
			{	10010, 9981, 9948, 10030, 10053, 9929, 9894, 10110, 10040, 10110,
				10019, 9961, 10078, 9959, 9966, 10030, 10089, 9908, 9938, 9890,
			};
			static constexpr auto M = 2;
			static constexpr VI_TM_TDIFF samples_multiple[] = { 990, }; // Samples that will be added M times at once.
			static constexpr VI_TM_TDIFF samples_exclude[] = { 200000 }; // Samples that will be excluded from the statistics.

			static constexpr auto exp_flt_cnt = std::size(samples_simple) + M * std::size(samples_multiple); // The total number of samples that will be counted.
			static const auto exp_flt_mean = 
				(	std::accumulate(std::cbegin(samples_simple), std::cend(samples_simple), 0.0) +
					static_cast<double>(M) * std::accumulate(std::cbegin(samples_multiple), std::cend(samples_multiple), 0.0)
				) / static_cast<VI_TM_FP>(exp_flt_cnt); // The mean value of the samples that will be counted.
#	ifdef VI_TM_STAT_USE_WELFORD
			const auto exp_flt_stddev = [] // The standard deviation of the samples that will be counted.
				{	const auto sum_squared_deviations =
					std::accumulate
					(	std::cbegin(samples_simple),
						std::cend(samples_simple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - exp_flt_mean; return FMA(d, d, i); }
					) +
					static_cast<double>(M) * std::accumulate
					(	std::cbegin(samples_multiple),
						std::cend(samples_multiple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - exp_flt_mean; return FMA(d, d, i); }
					);
					return std::sqrt(sum_squared_deviations / static_cast<VI_TM_FP>(exp_flt_cnt));
				}();
#	endif
			static constexpr char NAME[] = "dummy"; // The name of the measurement.

			const char *name = nullptr; // Name of the measurement to be filled in.
			vi_tmMeasurementStats_t md; // Measurement data to be filled in.
			std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> journal{ vi_tmJournalCreate(), vi_tmJournalClose }; // Journal for measurements, automatically closed on destruction.
			{	const auto m = vi_tmMeasurement(journal.get(), NAME); // Create a measurement 'NAME'.
				for (auto x : samples_simple) // Add simple samples one at a time.
				{	vi_tmMeasurementAdd(m, x);
				}

				vi_tmMeasurementStatsReset(&md);
				for (auto x : samples_multiple) // Add multiple samples M times at once.
				{	vi_tmMeasurementStatsAdd(&md, M * x, M);
				}

				vi_tmMeasurementMerge(m, &md); // Merge the statistics into the measurement.
				vi_tmMeasurementStatsReset(&md);

#	ifdef VI_TM_STAT_USE_WELFORD
				for (auto x : samples_exclude) // Add samples that will be excluded from the statistics.
				{	vi_tmMeasurementAdd(m, x, 1);
				}
#	endif
				vi_tmMeasurementGet(m, &name, &md); // Get the measurement data and name.
			}

			constexpr auto DBG_EPS = fp_EPSILON;
			(void)DBG_EPS;

			assert(name && std::strlen(name) + 1 == std::size(NAME) && 0 == std::strcmp(name, NAME));
#	ifdef VI_TM_STAT_USE_WELFORD
			assert(md.calls_ == std::size(samples_simple) + std::size(samples_multiple) + std::size(samples_exclude));
#ifdef VI_TM_STAT_USE_BASE
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple) + std::size(samples_exclude));
#endif
			assert(md.flt_amt_ == static_cast<VI_TM_FP>(exp_flt_cnt));
			assert(std::abs(md.flt_mean_ - exp_flt_mean) / exp_flt_mean < DBG_EPS);
			const auto s = std::sqrt(md.flt_ss_ / (md.flt_amt_));
			assert(std::abs(s - exp_flt_stddev) / exp_flt_stddev < DBG_EPS);
#	else
			assert(md.calls_ == std::size(samples_simple) + std::size(samples_multiple));
#ifdef VI_TM_STAT_USE_BASE
			assert(md.amt_ == std::size(samples_simple) + M * std::size(samples_multiple));
			assert(std::abs(static_cast<VI_TM_FP>(md.sum_) / md.amt_ - exp_flt_mean) < DBG_EPS);
#endif
#	endif
			return 0;
		}();
}
#endif // #if VI_TM_DEBUG

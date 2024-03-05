// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi/timing.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace
{
	namespace ch = std::chrono;
	using namespace std::literals;

	std::string time_format(ch::duration<double> s)
	{
		constexpr unsigned int SEC_MINUTE = 60;
		constexpr unsigned int SEC_HOUR = 60 * SEC_MINUTE;
		constexpr unsigned int SEC_DAY = 24 * SEC_HOUR;

		auto sec = s.count();
		if (sec < .0)
		{
			sec = .0;
		}

		std::string result(std::size("ddddDhhHmmMss.sS."), '\0');
		size_t len = 0;

		if (1e-6 > sec)
		{
			len = snprintf(result.data(), result.size(), "%.1fns", sec * 1e9);
		}
		else if (1e-3 > sec)
		{
			len = snprintf(result.data(), result.size(), "%.1fus", sec * 1e6);
		}
		else if (1.0 > sec)
		{
			len = snprintf(result.data(), result.size(), "%.1fms", sec * 1e3);
		}
		else if (SEC_MINUTE > sec)
		{
			len = snprintf(result.data(), result.size(), "%.1fs ", sec);
		}
		else
		{
			auto integral_part = static_cast<int>(std::round(sec));

			if (SEC_DAY <= integral_part)
			{
				len += std::snprintf(result.data(), result.size(), "%iD", integral_part / SEC_DAY);
			}
			integral_part %= SEC_DAY;

			if (len || SEC_HOUR <= integral_part)
			{
				len += std::snprintf(result.data() + len, result.size() - len, (len ? "%02ih" : "%ih"), integral_part / SEC_HOUR);
			}
			integral_part %= SEC_HOUR;

			if ((100 * SEC_DAY > sec) && (len || SEC_MINUTE <= integral_part))
			{
				len += std::snprintf(result.data() + len, result.size() - len, (len ? "%02im" : "%im"), integral_part / SEC_MINUTE);
			}
			integral_part %= SEC_MINUTE;

			if (SEC_DAY > sec)
			{
				len += snprintf(result.data() + len, result.size() - len, "%02is", integral_part);
			}
		}

		result.resize(len);
		return result;

	} // time_format(double sec)

#ifndef NDEBUG
	static const bool test_time_format = []
	{
		static constexpr struct { double t; const char* s; } itms[] =
		{
			{.0,			              "0.0ns"},
			{1e-12,			              "0.0ns"},
			{10e-12,		              "0.0ns"},
			{100e-12,		              "0.1ns"},
			{1.049e-9,		              "1.0ns"},
			{1.050e-9,		              "1.1ns"},
			{10e-9,			             "10.0ns"},
			{100e-9,		            "100.0ns"},
			{1e-6,			              "1.0us"},
			{10e-6,			             "10.0us"},
			{100e-6,		            "100.0us"},
			{1e-3,			              "1.0ms"},
			{10e-3,			             "10.0ms"},
			{100e-3,		            "100.0ms"},
			{1,				              "1.0s "},
			{60,			              "1m00s"},
			{1 * 60 * 60 - 1,		             "59m59s"},
			{1 * 60 * 60,		           "1h00m00s"},
			{1 * 60 * 60 + 0.1,	           "1h00m00s"},
			{24 * 60 * 60 - 1,	          "23h59m59s"},
			{24 * 60 * 60,		           "1D00h00m"},
			{24 * 60 * 60 + 59,	           "1D00h00m"},
			{10 * 24 * 60 * 60,	          "10D00h00m"},
			{((100 * 24) * 60) * 60 - 1,      "99D23h59m"},
			{((100 * 24) * 60) * 60,          "100D00h"},
			{((100 * 24) * 60 + 59) * 60,       "100D00h"},
		};

		for (const auto& v : itms)
		{
			auto s = time_format(ch::duration<double>(v.t));
			assert(s == v.s);
		}

		return true;
	}();
#endif

	ch::duration<double> seconds_per_tick()
	{
		auto wait_for_the_time_to_change = []
		{
			std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			[[maybe_unused]] volatile auto dummy_1 = vi_tmGetTicks(); // Preloading a function into cache
			[[maybe_unused]] volatile auto dummy_2 = ch::steady_clock::now(); // Preloading a function into cache

			auto last = ch::steady_clock::now();
			for (const auto first = ch::steady_clock::now(); first >= last; last = ch::steady_clock::now())
			{/**/}

			return std::tuple{ vi_tmGetTicks(), last };
		};

		const auto [tick1, time1] = wait_for_the_time_to_change();
		// Load the thread at 100% for 'interval'.
		constexpr auto interval = 256ms;
		for (auto stop = time1 + interval; ch::steady_clock::now() < stop;)
		{/**/}
		const auto [tick2, time2] = wait_for_the_time_to_change();

		return ch::duration<double>(time2 - time1) / (tick2 - tick1);
	}

	double overhead_ticks()
	{
		static constexpr auto CNT = 10'000U;
		static constexpr auto EXT = 10U;

		auto pure = []
		{
			std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.

			volatile auto e = vi_tmGetTicks(); // Preloading a function into cache
			auto s = vi_tmGetTicks();
			for (size_t cnt = CNT; cnt; --cnt)
			{
				e = vi_tmGetTicks();
			}
			return e - s;
		};

		auto dirty = []
		{
			std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.

			volatile auto e = vi_tmGetTicks(); // Preloading a function into cache
			auto s = vi_tmGetTicks();
			for (size_t cnt = CNT; cnt; --cnt)
			{
				e = vi_tmGetTicks(); //-V761

				// EXT calls
				e = vi_tmGetTicks();	e = vi_tmGetTicks();
				e = vi_tmGetTicks();	e = vi_tmGetTicks();
				e = vi_tmGetTicks();	e = vi_tmGetTicks();
				e = vi_tmGetTicks();	e = vi_tmGetTicks();
				e = vi_tmGetTicks();	e = vi_tmGetTicks();
			}
			return e - s;
		};

		return static_cast<double>(dirty() - pure()) / (EXT * CNT);
	}

	struct traits_t
	{
		struct itm_t
		{
			const char* name_{}; // Name
			vi_tmTicks_t total_{}; // Total ticks duration
			std::size_t amount_{}; // Number of measured units
			std::size_t calls_cnt_{}; // To account for overheads

			ch::duration<double> total_time_{}; // seconds
			ch::duration<double> average_{}; // seconds
			std::string total_txt_;
			std::string average_txt_;
			itm_t(const char* n, vi_tmTicks_t t, std::size_t a, std::size_t c) noexcept : name_{ n }, total_{ t }, amount_{ a }, calls_cnt_{ c } {}
		};

		std::vector<itm_t> meterages_;

		const ch::duration<double> tick_duration_ = seconds_per_tick();
		const double overheads_ = overhead_ticks(); // ticks
		std::size_t max_amount_{};
		std::size_t max_len_name_{};
		std::size_t max_len_total_{};
		std::size_t max_len_average_{};
		std::size_t max_len_amount_{};
	};

	int collector_meterages(const char* name, vi_tmTicks_t time, std::size_t amount, std::size_t calls_cnt, void* traits)
	{
		assert(traits);
		auto& v = *static_cast<traits_t*>(traits);
		assert(calls_cnt && amount >= calls_cnt);

		auto& itm = v.meterages_.emplace_back(name, time, amount, calls_cnt);

		v.max_len_name_ = std::max(v.max_len_name_, strlen(itm.name_));

		if (const auto total_over_ticks = v.overheads_ * itm.calls_cnt_; itm.total_ > total_over_ticks)
		{
			itm.total_time_ = (itm.total_ - total_over_ticks) * v.tick_duration_;
			itm.average_ = itm.total_time_ / itm.amount_;
		}
		else
		{
			// Leave zeros.
		}

		itm.total_txt_ = time_format(itm.total_time_);
		v.max_len_total_ = std::max(v.max_len_total_, itm.total_txt_.length());

		itm.average_txt_ = time_format(itm.average_);
		v.max_len_average_ = std::max(v.max_len_average_, itm.average_txt_.length());

		if (itm.amount_ > v.max_amount_)
		{
			v.max_amount_ = itm.amount_;
			auto max_len_amount = static_cast<std::size_t>(std::floor(std::log10(itm.amount_)));
			max_len_amount += max_len_amount / 3; // for thousand separators
			max_len_amount += 1;
			v.max_len_amount_ = max_len_amount;
		}

		return 1; // Continue enumerate.
	}

	struct meterage_comparator_t
	{
		uint32_t flags_{};

		explicit meterage_comparator_t(uint32_t flags) noexcept : flags_{ flags } {}

		bool operator ()(const traits_t::itm_t& l, const traits_t::itm_t& r) const {
			const bool desc = !(static_cast<uint32_t>(vi_tmSortAscending) & flags_);

			auto result = false;
			switch (flags_ & static_cast<uint32_t>(vi_tmSortMask))
			{
			case static_cast<uint32_t>(vi_tmSortByName):
				result = desc ? (strcmp(l.name_, r.name_) > 0) : (strcmp(l.name_, r.name_) < 0);
				break;
			case static_cast<uint32_t>(vi_tmSortByAmount):
				result = desc ? (l.amount_ > r.amount_) : (l.amount_ < r.amount_);
				break;
			case static_cast<uint32_t>(vi_tmSortByTime):
				result = desc ? (l.total_time_ > r.total_time_) : (l.total_time_ < r.total_time_);
				break;
			case static_cast<uint32_t>(vi_tmSortBySpeed):
			default:
				result = desc ? (l.average_ > r.average_) : (l.average_ < r.average_);
				break;
			}
			return result;
		}
	};

	struct meterage_format_t
	{
		const traits_t& traits_;
		const vi_tmLogSTR fn_;
		void* const data_;
		std::size_t& n_;

		meterage_format_t(std::size_t& n, traits_t& traits, vi_tmLogSTR fn, void* data) :traits_{ traits }, fn_{ fn }, data_{ data }, n_{ n } {}

		int operator ()(int init_t, const traits_t::itm_t& i) const
		{
			std::ostringstream str;
			struct thousands_sep_facet_t final : std::numpunct<char>
			{
				char do_thousands_sep() const override { return '\''; }
				std::string do_grouping() const override { return "\x3"; }
			};
			str.imbue(std::locale(str.getloc(), new thousands_sep_facet_t));

			str << std::setfill(n_++ % 2 ? ' ' : '.') << std::left << std::setw(traits_.max_len_name_) << i.name_ << ": ";
			str << std::setfill(' ') << std::right << std::setw(traits_.max_len_total_) << i.total_txt_ << " / ";
			str << std::setw(traits_.max_len_amount_) << i.amount_ << " = ";
			str << std::setw(traits_.max_len_average_) << i.average_txt_ << "\n";

			auto result = str.str();
			assert(traits_.max_len_name_ + 2 + traits_.max_len_total_ + 3 + traits_.max_len_amount_ + 3 + traits_.max_len_average_ + 1 == result.size());

			return init_t + fn_(result.c_str(), data_);
		}
	};
} // namespace {

VI_TM_API int VI_TM_CALL vi_tmReport(vi_tmLogSTR fn, void* data, std::uint32_t flags)
{
	traits_t traits;
	vi_tmResults(collector_meterages, &traits);

	std::sort(traits.meterages_.begin(), traits.meterages_.end(), meterage_comparator_t{ flags });

	int ret = 0;
	std::ostringstream str;
	if (flags & static_cast<uint32_t>(vi_tmShowOverhead))
		str << "Timer overhead: " << time_format(traits.tick_duration_ * traits.overheads_) << " per measurement. ";

	if (flags & static_cast<uint32_t>(vi_tmShowUnit))
		str << "One tick corresponds: " << time_format(traits.tick_duration_) << ". ";

	str << '\n';
	auto s = str.str();
	ret += fn(s.c_str(), data);

	std::size_t n = 0;
	return std::accumulate(traits.meterages_.begin(), traits.meterages_.end(), ret, meterage_format_t{ n, traits, fn, data });
}

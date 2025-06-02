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
#include "build_number_maker.h"
#include "misc.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace
{
	constexpr char TitleName[] = "Name";
	constexpr char TitleAverage[] = "Avg.";
	constexpr char TitleTotal[] = "Total";
	constexpr char TitlePrecision[] = "CV";
	constexpr char TitleAmount[] = "Amount";
	constexpr char Ascending[] = " [^]";
	constexpr char Descending[] = " [v]";
	constexpr char Insignificant[] = "<insig>";
	constexpr char NotAvailable[] = "<n/a>";

	constexpr unsigned char DURATION_PREC = 2;
	constexpr unsigned char DURATION_DEC = 1;

	template<unsigned char P, unsigned char D>
	struct duration_t: std::chrono::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T &&v): std::chrono::duration<double>{ std::forward<T>(v) } {}
		duration_t() = default;

		[[nodiscard]] friend std::string to_string(const duration_t &d)
		{	return misc::to_string(d.count(), P, D) + "s";
		}
		[[nodiscard]] friend bool operator<(const duration_t &l, const duration_t &r)
		{	return l.count() < r.count() && to_string(l) != to_string(r);
		}
	};

	struct metering_t
	{	std::string_view name_;
		std::string sum_txt_{ NotAvailable };
		duration_t<DURATION_PREC, DURATION_DEC> sum_{ .0 }; // seconds
		std::string average_txt_{ NotAvailable };
		duration_t<DURATION_PREC, DURATION_DEC> average_{ .0 }; // seconds
#if defined(VI_TM_STAT_USE_WELFORD)
		double sd_{ .0 }; // standard deviation in ticks
		std::string cv_txt_{ "" }; // Coefficient of Variation (CV) in percent
#endif
		std::size_t amt_{}; // Number of measured units

		metering_t(const char *name, const vi_tmMeasuringRAW_t &meas) noexcept;
	};

	metering_t::metering_t(const char *name, const vi_tmMeasuringRAW_t &meas) noexcept
	:	name_{ name },
		amt_{ meas.amt_ }
	{	assert(amt_ >= meas.calls_);
		assert(!!amt_ == !!meas.calls_);

		if (0 != amt_)
		{	auto &props = misc::properties_t::props();
#ifdef VI_TM_STAT_USE_WELFORD
			if (const auto ave = meas.flt_mean_ - props.clock_latency_ticks_; ave <= props.clock_resolution_ticks_ / std::sqrt(meas.calls_))
			{	sum_txt_ = Insignificant;
				average_txt_ = Insignificant;
			}
			else
			{	average_ = ave * props.seconds_per_tick_;
				average_txt_ = to_string(average_);
				sum_ = average_ * amt_;
				sum_txt_ = to_string(sum_);
				if (meas.calls_ > 1)
				{	assert(amt_ > 1);
					sd_ = std::sqrt(meas.flt_ss_ / (meas.flt_cnt_ - 1));
					cv_txt_ = misc::to_string(sd_ / ave * 100.0, 2, 0) + "%";
				}
			}
#else
			if (const auto ticks = static_cast<double>(meas.sum_) - props.clock_latency_ticks_ * static_cast<double>(meas.calls_);
				ticks <= props.clock_resolution_ticks_ * std::sqrt(meas.calls_)
			)
			{	sum_txt_ = Insignificant;
				average_txt_ = Insignificant;
			}
			else
			{	sum_ = props.seconds_per_tick_ * ticks;
				sum_txt_ = to_string(sum_);
				average_ = sum_ / amt_;
				average_txt_ = to_string(average_);
			}
#endif
		}
	}

	template<vi_tmReportFlags_e E> auto make_tuple(const metering_t &v);

	template<> auto make_tuple<vi_tmSortByName>(const metering_t &v)
	{	return std::tuple{ v.name_, v.average_, v.sum_, v.amt_ };
	}
	template<> auto make_tuple<vi_tmSortBySpeed>(const metering_t &v)
	{	return std::tuple{ v.average_, v.sum_, v.amt_, v.name_ };
	}
	template<> auto make_tuple<vi_tmSortByTime>(const metering_t &v)
	{	return std::tuple{ v.sum_, v.average_, v.amt_, v.name_ };
	}
	template<> auto make_tuple<vi_tmSortByAmount>(const metering_t &v)
	{	return std::tuple{ v.amt_, v.average_, v.sum_, v.name_ };
	}

	template<vi_tmReportFlags_e E> bool less(const metering_t &l, const metering_t &r)
	{	return make_tuple<E>(l) < make_tuple<E>(r);
	}

	class comparator_t
	{	bool (*pr_)(const metering_t &, const metering_t &);
		const bool ascending_;
	public:
		explicit comparator_t(unsigned flags) noexcept
			: ascending_{ 0 != (flags & vi_tmSortAscending) }
		{
			switch (flags & vi_tmSortMask)
			{
			default:
				assert(false);
				[[fallthrough]];
			case vi_tmSortByName:
				pr_ = less<vi_tmSortByName>;
				break;
			case vi_tmSortByAmount:
				pr_ = less<vi_tmSortByAmount>;
				break;
			case vi_tmSortByTime:
				pr_ = less<vi_tmSortByTime>;
				break;
			case vi_tmSortBySpeed:
				pr_ = less<vi_tmSortBySpeed>;
				break;
			}
		}
		bool operator ()(const metering_t &l, const metering_t &r) const
		{	return ascending_ ? pr_(l, r): pr_(r, l);
		}
	};

	struct formatter_t
	{	static constexpr auto fill_symbol = '.';
		const std::size_t number_len_;
		const unsigned flags_;
		const unsigned guideline_interval_;

		std::size_t max_len_name_{ std::size(TitleName) - 1 };
		std::size_t max_len_average_{ std::size(TitleAverage) - 1 };
#ifdef VI_TM_STAT_USE_WELFORD
		std::size_t max_len_precision_{ std::size(TitlePrecision) - 1 };
#endif
		std::size_t max_len_total_{ std::size(TitleTotal) - 1 };
		std::size_t max_len_amount_{ std::size(TitleAmount) - 1 };
		mutable std::size_t n_{ 0 };

		formatter_t(const std::vector<metering_t> &itms, unsigned flags);
		int print_header(const vi_tmRptCb_t fn, void *data) const;
		int print_metering(const metering_t &i, const vi_tmRptCb_t fn, void *data) const;
	};

	std::vector<metering_t> get_meterings(VI_TM_HJOUR journal_handle)
	{	std::vector<metering_t> result;
		vi_tmMeasuringEnumerate
		(	journal_handle,
			[](VI_TM_HMEAS h, void *callback_data)
			{	const char *name;
				vi_tmMeasuringRAW_t meas;
				vi_tmMeasuringGet(h, &name, &meas);
				static_cast<std::vector<metering_t> *>(callback_data)->emplace_back(name, std::move(meas));
				return 0; // Ok, continue enumerate.
			},
			&result
		);
		return result;
	}

	int print_props(vi_tmRptCb_t fn, void *data, unsigned flags)
	{	assert(!!fn);
		int result = 0;
		if (flags & vi_tmShowMask)
		{	std::ostringstream str;
			auto &props = misc::properties_t::props();

			auto to_string = [](auto d) { return misc::to_string(d, DURATION_PREC, DURATION_DEC) + "s. "; };
			if (flags & vi_tmShowResolution)
			{	str << "Resolution: " << to_string(props.seconds_per_tick_.count() * props.clock_resolution_ticks_);
			}
			if (flags & vi_tmShowDuration)
			{	str << "Duration: " << to_string(props.all_latency_.count());
			}
			if (flags & vi_tmShowUnit)
			{	str << "One tick: " << to_string(props.seconds_per_tick_.count());
			}
			if (flags & vi_tmShowOverhead)
			{	str << "Additive: " << to_string(props.seconds_per_tick_.count() * props.clock_latency_ticks_);
			}

			str << '\n';
			result += fn(str.str().c_str(), data);
		}

		return result;
	}
} // namespace

formatter_t::formatter_t(const std::vector<metering_t> &itms, unsigned flags)
:	number_len_{ itms.empty() ? 1U : 1U + static_cast<std::size_t>(std::floor(std::log10(itms.size()))) },
	flags_{ flags },
	guideline_interval_{ itms.size() > 4U ? 3U : 0U }
{	const auto mark = ((flags & vi_tmSortAscending) ? std::size(Ascending) : std::size(Descending)) - 1U;
	switch (flags_ & vi_tmSortMask)
	{
	default:
		assert(false);
		[[fallthrough]];
	case vi_tmSortByName:
		max_len_name_ += mark;
		break;
	case vi_tmSortByAmount:
		max_len_amount_ += mark;
		break;
	case vi_tmSortByTime:
		max_len_total_ += mark;
		break;
	case vi_tmSortBySpeed:
		max_len_average_ += mark;
		break;
	}

	std::size_t max_amount = 0U;
	for (auto &itm : itms)
	{	max_len_total_ = std::max(max_len_total_, itm.sum_txt_.length());
		max_len_average_ = std::max(max_len_average_, itm.average_txt_.length());
		max_len_name_ = std::max(max_len_name_, itm.name_.length());
#if defined(VI_TM_STAT_USE_WELFORD)
		max_len_precision_ = std::max(max_len_precision_, itm.cv_txt_.length());
#endif
		if (itm.amt_ > max_amount)
		{	max_amount = itm.amt_;
			auto len = static_cast<std::size_t>(std::floor(std::log10(max_amount)));
			len += len / 3U; // for thousand separators
			len += 1U;
			max_len_amount_ = std::max(max_len_amount_, len);
		}
	}
}

int formatter_t::print_header(const vi_tmRptCb_t fn, void *data) const
{	
	if (flags_ & vi_tmHideHeader)
	{	return 0;
	}

	auto sort = vi_tmSortByName;
	switch (auto s = flags_ & vi_tmSortMask)
	{
	case vi_tmSortByName:
	case vi_tmSortBySpeed:
	case vi_tmSortByAmount:
	case vi_tmSortByTime:
		sort = static_cast<vi_tmReportFlags_e>(s);
		break;
	default:
		assert(false);
		break;
	}

	auto title = [sort, order = (flags_ & vi_tmSortAscending) ? Ascending : Descending]
	(const char *name, vi_tmReportFlags_e s)
		{	return std::string{ name } + (sort == s ? order : "");
		};

	std::ostringstream str;
	str <<
		std::setw(number_len_) << "#" << "  " << std::setfill(fill_symbol) << std::left <<
		std::setw(max_len_name_) << title(TitleName, vi_tmSortByName) << ": " << std::setfill(' ') << std::right <<
		std::setw(max_len_average_) << title(TitleAverage, vi_tmSortBySpeed) <<
#if defined(VI_TM_STAT_USE_WELFORD)
		" (" << std::setw(max_len_precision_) << TitlePrecision << ")"
#endif
		" [" <<
		std::setw(max_len_total_) << title(TitleTotal, vi_tmSortByTime) << " / " <<
		std::setw(max_len_amount_) << title(TitleAmount, vi_tmSortByAmount) << "]"
		"\n";

	const auto result = str.str();
#ifndef NDEBUG	
	auto len = number_len_ + 2 + max_len_name_ + 2 + max_len_average_ + 2 + max_len_total_ + 3 + max_len_amount_ + 1 + 1;
#if defined(VI_TM_STAT_USE_WELFORD)
		len += max_len_precision_ + 1 + 2;
#	endif
	assert(len == result.size());
#endif
	return fn(result.c_str(), data);
}

int formatter_t::print_metering(const metering_t &i, const vi_tmRptCb_t fn, void *data) const
{
	std::ostringstream str;
	str.imbue(std::locale(str.getloc(), new misc::space_out));

	n_++;
	const char fill = (guideline_interval_ && (0 == n_ % guideline_interval_)) ? fill_symbol : ' ';
	str <<
		std::setw(number_len_) << n_ << ". " << std::setfill(fill) << std::left <<
		std::setw(max_len_name_) << i.name_ << ": " << std::setfill(' ') << std::right <<
		std::setw(max_len_average_) << i.average_txt_ <<
#if defined(VI_TM_STAT_USE_WELFORD)
		" (" << std::setw(max_len_precision_) << i.cv_txt_ << ")" <<
#endif
		" [" <<
		std::setw(max_len_total_) << i.sum_txt_ << " / " <<
		std::setw(max_len_amount_) << i.amt_ << "]"
		"\n";

	const auto result = str.str();
#ifndef NDEBUG	
	{	
		auto len = number_len_ + 2 + max_len_name_ + 2 + max_len_average_ + 2 + max_len_total_ + 3 + max_len_amount_ + 1 + 1;
#if defined(VI_TM_STAT_USE_WELFORD)
		len += max_len_precision_ + 1 + 2;
#	endif
		assert(len == result.size());
	}
#endif
	return fn(result.c_str(), data);
}

int VI_TM_CALL vi_tmReport(VI_TM_HJOUR journal_handle, unsigned flags, vi_tmRptCb_t fn, void *data)
{	
	if (nullptr == fn)
	{	fn = [](const char *str, void *data) { return std::fputs(str, static_cast<std::FILE *>(data)); };
		if (nullptr == data)
		{	data = stdout;
		}
	}

	(void) misc::properties_t::props(); // Preventing deadlock in traits_t::results_callback().
	int result = print_props(fn, data, flags);

	std::vector<metering_t> metering_entries = get_meterings(journal_handle);
	const formatter_t formatter{ metering_entries, flags };
	result += formatter.print_header(fn, data);

	std::sort(metering_entries.begin(), metering_entries.end(), comparator_t{ flags });
	for (const auto &itm : metering_entries)
	{	result += formatter.print_metering(itm, fn, data);
	}

	return result;
}

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

#ifdef _WIN32
#	include <windows.h> // For GetStdHandle(STD_OUTPUT_HANDLE)
#endif

namespace
{
	constexpr char TitleNumber[] = "#";
	constexpr char TitleName[] = "Name";
	constexpr char TitleAverage[] = "Avg.";
	constexpr char TitleTotal[] = "Sum.";
	constexpr char TitleCV[] = "CV";
	constexpr char TitleAmount[] = "Amt.";
	constexpr char TitleMin[] = "Min.";
	constexpr char TitleMax[] = "Max.";
	constexpr char Ascending[] = " [^]";
	constexpr char Descending[] = " [v]";
	constexpr char Insignificant[] = "<ins>"; // insignificant
	constexpr char Excessive[] = "<exc>"; // excessively
	constexpr char NotAvailable[] = "n/a";

	constexpr unsigned char DURATION_PREC = 2;
	constexpr unsigned char DURATION_DEC = 1;

	template<unsigned char P, unsigned char D>
	struct duration_t: std::chrono::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T &&v): std::chrono::duration<double>{ std::forward<T>(v) } {}
		duration_t() = default;

		[[nodiscard]] friend std::string to_string(const duration_t &d) { return misc::to_string(d.count(), P, D) + "s"; }
		[[nodiscard]] friend bool operator<(const duration_t &l, const duration_t &r)
			{	return l.count() < r.count() && to_string(l) != to_string(r);
			}
	};

	struct metering_t
	{	std::string name_; // Name of the measured.
		duration_t<DURATION_PREC, DURATION_DEC> sum_{}; // seconds
		std::string sum_txt_{ NotAvailable };
		duration_t<DURATION_PREC, DURATION_DEC> average_{}; // seconds
		std::string average_txt_{ NotAvailable };
		std::size_t amt_{}; // Number of measured units
		std::string amt_txt_{ "0" };
#ifdef VI_TM_STAT_USE_WELFORD
		duration_t<DURATION_PREC, DURATION_DEC> min_{}; // Minimum time in seconds
		std::string min_txt_{ NotAvailable };
		duration_t<DURATION_PREC, DURATION_DEC> max_{}; // Maximum time in seconds
		std::string max_txt_{ NotAvailable };
		double cv_{}; // Coefficient of Variation.
		std::string cv_txt_{ NotAvailable }; // Coefficient of Variation (CV) in percent
#endif

		metering_t(const char *name, const vi_tmMeasurementStats_t &meas, unsigned flags) noexcept;
	};

	template<vi_tmReportFlags_e E> auto make_tuple(const metering_t &v);

	template<> auto make_tuple<vi_tmSortByName>(const metering_t &v)
	{	return std::tie( v.name_, v.average_, v.sum_, v.amt_ );
	}
	template<> auto make_tuple<vi_tmSortBySpeed>(const metering_t &v)
	{	return std::tie( v.average_, v.sum_, v.amt_, v.name_ );
	}
	template<> auto make_tuple<vi_tmSortByTime>(const metering_t &v)
	{	return std::tie( v.sum_, v.average_, v.amt_, v.name_ );
	}
	template<> auto make_tuple<vi_tmSortByAmount>(const metering_t &v)
	{	return std::tie( v.amt_, v.average_, v.sum_, v.name_ );
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
	{	static constexpr auto UNDERSCORE = '.';
		const std::size_t max_len_number_;
		const unsigned flags_;
		const unsigned guideline_interval_;

		std::size_t max_len_name_{};
		std::size_t max_len_average_{};
#ifdef VI_TM_STAT_USE_WELFORD
		std::size_t max_len_min_{};
		std::size_t max_len_max_{};
		std::size_t max_len_cv_{};
#endif
		std::size_t max_len_total_{};
		std::size_t max_len_amount_{};
		mutable std::size_t n_{ 0 };

		formatter_t(const std::vector<metering_t> &itms, unsigned flags);
		int print_header(const vi_tmReportCb_t fn, void *data) const;
		int print_metering(const metering_t &i, const vi_tmReportCb_t fn, void *data) const;

		std::size_t width_column(vi_tmReportFlags_e clmn) const;
		const char* mark_column(vi_tmReportFlags_e clmn) const;
		std::string item_column(vi_tmReportFlags_e clmn, const char* txt = nullptr) const;
	};

	std::vector<metering_t> get_meterings(VI_TM_HJOUR journal_handle, unsigned flags)
	{	std::vector<metering_t> result;
		auto data = std::tie(result, flags);
		using data_t = decltype(data);
		vi_tmMeasurementEnumerate
		(	journal_handle,
			[](VI_TM_HMEAS h, void *callback_data)
			{	const char *name;
				vi_tmMeasurementStats_t meas;
				vi_tmMeasurementGet(h, &name, &meas);
				auto& [vec, flags] = *static_cast<data_t*>(callback_data);
				vec.emplace_back(name, std::move(meas), flags);
				return 0; // Ok, continue enumerate.
			},
			&data
		);
		return result;
	}

	int print_props(vi_tmReportCb_t fn, void *data, unsigned flags)
	{	assert(!!fn);
		int result = 0;
		if (flags & vi_tmShowMask)
		{	std::ostringstream str;
			auto &props = misc::properties_t::props();

			auto to_string = [](auto d) { return misc::to_string(d, DURATION_PREC, DURATION_DEC) + "s. "; };
			if (flags & vi_tmShowCorrected)
			{	str << (flags & vi_tmDoNotSubtractOverhead? "": "Corrected. ");
			}
			if (flags & vi_tmShowResolution)
			{	str << "Resolution: " << to_string(props.seconds_per_tick_.count() * props.clock_resolution_ticks_);
			}
			if (flags & vi_tmShowDuration)
			{	str << "Duration: " << to_string(props.duration_.count());
			}
			if (flags & vi_tmShowDurationEx)
			{	str << "Duration ex: " << to_string(props.duration_ex_.count());
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

	vi_tmReportFlags_e to_sort_flag(unsigned flags_)
	{ // Convert flags_ to vi_tmReportFlags_e type, ensuring it is one of the defined sorting types.
		auto result = vi_tmSortByName;
		switch (auto s = flags_ & vi_tmSortMask)
		{
		case vi_tmSortByName:
		case vi_tmSortBySpeed:
		case vi_tmSortByAmount:
		case vi_tmSortByTime:
			result = static_cast<vi_tmReportFlags_e>(s);
			break;
		default:
			assert(false);
			break;
		}
		return result;
	}

} // namespace

metering_t::metering_t(const char *name, const vi_tmMeasurementStats_t &meas, unsigned flags) noexcept
:	name_{ name },
	amt_{ meas.amt_ }
{	assert(amt_ >= meas.calls_);
	assert(!!amt_ == !!meas.calls_);

	if (0 != amt_)
	{	auto &props = misc::properties_t::props();

		{	std::ostringstream str;
			str.imbue(std::locale(str.getloc(), new misc::space_out));
			str << amt_;
			amt_txt_ = str.str();
		}

		const bool subtract_overhead{ 0U == (flags & vi_tmDoNotSubtractOverhead) };
#ifdef VI_TM_STAT_USE_WELFORD
		const auto correction = (subtract_overhead ? props.clock_latency_ticks_ * meas.flt_calls_ / meas.flt_amt_ : 0.0);
		const auto limit = props.clock_resolution_ticks_ / std::sqrt(meas.flt_amt_);

		assert(meas.min_ != INFINITY && meas.max_ != -INFINITY);
		assert(meas.flt_amt_ && meas.flt_calls_); // Since amt_ is not zero.
		assert(meas.flt_calls_ <= meas.flt_amt_);
		if (1 < meas.calls_)
		{
			if (const auto v = meas.min_ - correction; v > limit)
			{	min_ = props.seconds_per_tick_ * v;
				min_txt_ = to_string(min_);
			}
			else
			{	min_txt_ = Insignificant;
			}

			if (const auto v = meas.max_ - correction; v > limit)
			{	max_ = props.seconds_per_tick_ * v;
				max_txt_ = to_string(max_);
			}
			else
			{	max_txt_ = Insignificant;
			}
		}
		else
		{	assert(meas.min_ == meas.max_);
		}

		if (const auto ave = meas.flt_mean_ - correction; ave <= limit)
		{	sum_txt_ = Insignificant;
			average_txt_ = Insignificant;
		}
		else
		{	average_ = ave * props.seconds_per_tick_;
			average_txt_ = to_string(average_);
			sum_ = average_ * amt_;
			sum_txt_ = to_string(sum_);
			if (meas.calls_ >= 2)
			{	assert(amt_ >= 2); // Zero amt_ values are not included in meas.calls_.
				assert(meas.flt_amt_ >= 2); // The first two measurements cannot be filtered out.
				cv_ = std::sqrt(meas.flt_ss_ / static_cast<double>(meas.flt_amt_ - 1)) / ave;
				if (const auto cv = std::round(cv_ * 100.0); cv < 1.0)
				{	cv_txt_ = "<1"; // Coefficient of Variation (CV) is too low.
				}
				else if (cv >= 100.0)
				{	cv_txt_ = Excessive; // Coefficient of Variation (CV) is too high.
				}
				else
				{	cv_txt_ = misc::to_string(cv, 2, 0);
					assert(cv_txt_.back() == ' '); // In these conditions, the last character is always a space.
					cv_txt_.back() = '%'; // Replace last char with '%'.
				}
			}
		}
#else
		if (const auto ticks = static_cast<double>(meas.sum_) - (subtract_overhead? props.clock_latency_ticks_ * static_cast<double>(meas.calls_): 0.0);
			ticks <= props.clock_resolution_ticks_ * std::sqrt(amt_)
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

formatter_t::formatter_t(const std::vector<metering_t> &itms, unsigned flags)
:	max_len_number_{ std::max(itms.empty() ? 1U : 1U + size_t(floor(log10(itms.size()))), std::size(TitleNumber) - 1U) },
	flags_{ flags },
	guideline_interval_{ itms.size() > 4U ? 3U : 0U }
{	
	if ((flags & vi_tmHideHeader) == 0)
	{
		max_len_name_ = std::size(TitleName) - 1;
		max_len_average_ = std::size(TitleAverage) - 1;
#ifdef VI_TM_STAT_USE_WELFORD
		max_len_cv_ = std::size(TitleCV) - 1;
		max_len_min_ = std::size(TitleMin) - 1;
		max_len_max_ = std::size(TitleMax) - 1;
#endif
		max_len_total_ = std::size(TitleTotal) - 1;
		max_len_amount_ = std::size(TitleAmount) - 1;
	}

	for (auto &itm : itms)
	{	max_len_name_ = std::max(max_len_name_, itm.name_.length());
		max_len_average_ = std::max(max_len_average_, itm.average_txt_.length());
#ifdef VI_TM_STAT_USE_WELFORD
		max_len_cv_ = std::max(max_len_cv_, itm.cv_txt_.length());
		max_len_min_ = std::max(max_len_min_, itm.min_txt_.length());
		max_len_max_ = std::max(max_len_max_, itm.max_txt_.length());
#endif
		max_len_total_ = std::max(max_len_total_, itm.sum_txt_.length());
		max_len_amount_ = std::max(max_len_amount_, itm.amt_txt_.length());
	}
}

std::size_t formatter_t::width_column(vi_tmReportFlags_e clmn) const
{	std::size_t result = 0;
	std::size_t title_len = 0;
	switch (clmn)
	{	case vi_tmSortByName:
			result = static_cast<int>(max_len_name_);
			title_len = std::size(TitleName) - 1U;
			break;
		case vi_tmSortBySpeed:
			result = static_cast<int>(max_len_average_);
			title_len = std::size(TitleAverage) - 1U;
			break;
		case vi_tmSortByTime:
			result = static_cast<int>(max_len_total_);
			title_len = std::size(TitleTotal) - 1U;
			break;
		case vi_tmSortByAmount:
			result = static_cast<int>(max_len_amount_);
			title_len = std::size(TitleAmount) - 1U;
			break;
		default:
			assert(false);
			break;
	}

	if (to_sort_flag(flags_) == clmn)
	{	title_len += flags_ & vi_tmSortAscending ? std::size(Ascending) - 1U : std::size(Descending) - 1U;
		result = std::max(title_len, result);
	}

	return result;
}

const char* formatter_t::mark_column(vi_tmReportFlags_e clmn) const
{	return to_sort_flag(flags_) == clmn ? (flags_ & vi_tmSortAscending ? Ascending : Descending) : "";
};

std::string formatter_t::item_column(vi_tmReportFlags_e clmn, const char* txt) const
{	std::ostringstream str;
	if (!txt)
	{	switch (clmn)
		{
		case vi_tmSortByName:
			txt = TitleName;
			break;
		case vi_tmSortBySpeed:
			txt = TitleAverage;
			break;
		case vi_tmSortByTime:
			txt = TitleTotal;
			break;
		case vi_tmSortByAmount:
			txt = TitleAmount;
			break;
		default:
			assert(false);
			break;
		}

		str << txt << mark_column(clmn);
	}
	else
	{	str << txt;
	}
	return str.str();
}

int formatter_t::print_header(const vi_tmReportCb_t fn, void *data) const
{	
	if (flags_ & vi_tmHideHeader)
	{	return 0;
	}

	std::ostringstream str;
	str <<
		std::setw(max_len_number_) << TitleNumber << "  " <<
		std::left << std::setfill(UNDERSCORE) <<
		std::setw(width_column(vi_tmSortByName)) << item_column(vi_tmSortByName) << ": " <<
		std::right << std::setfill(' ') <<
		std::setw(width_column(vi_tmSortBySpeed)) << item_column(vi_tmSortBySpeed) << " "
#ifdef VI_TM_STAT_USE_WELFORD
		"(" << std::setw(max_len_cv_) << TitleCV << ") "
		"(" << std::setw(max_len_min_) << TitleMin << " - " << std::setw(max_len_max_) << TitleMax << ") "
#endif
		"[" << std::setw(width_column(vi_tmSortByTime)) << item_column(vi_tmSortByTime) << " / " <<
		std::setw(width_column(vi_tmSortByAmount)) << item_column(vi_tmSortByAmount) << "]\n";

	return fn(str.str().c_str(), data);
}

int formatter_t::print_metering(const metering_t &i, const vi_tmReportCb_t fn, void *data) const
{	std::ostringstream str;
	str.imbue(std::locale(str.getloc(), new misc::space_out));

	n_++;
	const char fill = (guideline_interval_ && (0 == n_ % guideline_interval_)) ? UNDERSCORE : ' ';
	str <<
		std::setw(max_len_number_) << n_ << ". " << 
		std::left << std::setfill(fill) <<
		std::setw(width_column(vi_tmSortByName)) << i.name_ << ": " <<
		std::right << std::setfill(' ') <<
		std::setw(width_column(vi_tmSortBySpeed)) << i.average_txt_ << " "
#ifdef VI_TM_STAT_USE_WELFORD
		"(" << std::setw(max_len_cv_) << i.cv_txt_ << ") "
		"(" << std::setw(max_len_min_) << i.min_txt_ << " - " << std::setw(max_len_max_) << i.max_txt_ << ") "
#endif
		"[" << std::setw(width_column(vi_tmSortByTime)) << i.sum_txt_ << " / " <<
		std::setw(width_column(vi_tmSortByAmount)) << i.amt_ << "]\n";

	return fn(str.str().c_str(), data);
}

int VI_TM_CALL vi_tmReport(VI_TM_HJOUR journal_handle, unsigned flags, vi_tmReportCb_t fn, void *data)
{	
	if (nullptr == fn)
	{	fn = [](const char *str, void *data) { return std::fputs(str, static_cast<std::FILE *>(data)); };
		if (nullptr == data)
		{	data = stdout;
		}
	}

	int result = print_props(fn, data, flags);

	std::vector<metering_t> metering_entries = get_meterings(journal_handle, flags);
	const formatter_t formatter{ metering_entries, flags };
	result += formatter.print_header(fn, data);

	std::sort(metering_entries.begin(), metering_entries.end(), comparator_t{ flags });
	for (const auto &itm : metering_entries)
	{	result += formatter.print_metering(itm, fn, data);
	}

	return result;
}

int VI_SYS_CALL vi_tmReportCb(const char *str, void *data)
{
#ifdef _WIN32
	// If the output is directed to stdout and the standard output handle is not available, return 0.
	// In /SUBSYSTEM:WINDOWS, stdout does not work by default.
	if (static_cast<FILE*>(data) == stdout && nullptr == GetStdHandle(STD_OUTPUT_HANDLE))
	{	return 0;
	}
#endif
	return fputs(str, (FILE *)data);
}

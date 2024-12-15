// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/********************************************************************\
'vi_timing' is a compact library designed for measuring the execution time of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was developed for experimental and educational purposes.
Please temper your expectations accordingly. If you encounter any bugs or have
suggestions for improvements, kindly contact me at programmer.amateur@proton.me.

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
#include "internal.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace
{
	using report_flags_t = std::underlying_type_t<vi_tmReportFlags_e>;

#if __cpp_lib_to_underlying
	using std::to_underlying;
#else
	template< class Enum >
	constexpr auto to_underlying(Enum e) noexcept { return static_cast<std::underlying_type_t<Enum>>(e); }
#endif

	constexpr char TitleName[] = "Name";
	constexpr char TitleAverage[] = "Average";
	constexpr char TitleTotal[] = "Total";
	constexpr char TitleAmount[] = "Amount";
	constexpr char Ascending[] = " [^]";
	constexpr char Descending[] = " [v]";
	constexpr char Insignificant[] = "<insig>";
	constexpr char NotAvailable[] = "<n/a>";

	struct itm_t
	{
		std::string_view name_; // Name
		misc::duration_t total_time_{ .0 }; // seconds
		misc::duration_t average_{ .0 }; // seconds
		std::size_t amount_{}; // Number of measured units
		std::string average_txt_{ NotAvailable };
		std::string total_txt_{ NotAvailable };

		itm_t(const char *n, vi_tmTicks_t total_time, std::size_t a, std::size_t c) noexcept;
	};

	itm_t::itm_t(const char *name, vi_tmTicks_t total_time, std::size_t amount, std::size_t calls_cnt) noexcept
		: name_{ name },
		amount_{ amount }
	{
		assert(amount >= calls_cnt);
		assert((0 == calls_cnt) == (0 == amount));

		if (0 == amount)
		{
		}
		else if
			(const auto burden = props().clock_latency_ * calls_cnt;
				total_time <= burden + props().clock_resolution_ * std::sqrt(calls_cnt)
				)
		{
			total_txt_ = Insignificant;
			average_txt_ = Insignificant;
		}
		else
		{
			total_time_ = props().tick_duration_ * (total_time - burden);
			average_ = total_time_ / amount_;
			average_txt_ = to_string(average_);
			total_txt_ = to_string(total_time_);
		}
	}

	int results_callback(const char *name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt, void *data)
	{
		assert(data);
		static_cast<std::vector<itm_t> *>(data)->emplace_back(name, total, amount, calls_cnt);
		return 1; // Continue enumerate.
	}

	template<vi_tmReportFlags_e E> auto make_tuple(const itm_t &v);
	template<> auto make_tuple<vi_tmSortByName>(const itm_t &v)
	{
		return std::tuple{ v.name_, v.average_, v.total_time_, v.amount_ };
	}
	template<> auto make_tuple<vi_tmSortBySpeed>(const itm_t &v)
	{
		return std::tuple{ v.average_, v.total_time_, v.amount_, v.name_ };
	}
	template<> auto make_tuple<vi_tmSortByTime>(const itm_t &v)
	{
		return std::tuple{ v.total_time_, v.average_, v.amount_, v.name_ };
	}
	template<> auto make_tuple<vi_tmSortByAmount>(const itm_t &v)
	{
		return std::tuple{ v.amount_, v.average_, v.total_time_, v.name_ };
	}

	template<vi_tmReportFlags_e E> bool less(const itm_t &l, const itm_t &r)
	{
		return make_tuple<E>(r) < make_tuple<E>(l);
	}

	class meterage_comparator_t
	{
		bool (*pr_)(const itm_t &, const itm_t &) = &less<vi_tmSortByName>;
		const bool ascending_;
	public:
		explicit meterage_comparator_t(unsigned flags) noexcept
			: ascending_{ 0 != (flags & vi_tmSortAscending) }
		{
			switch (flags & vi_tmSortMask)
			{
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
			default:
				assert(false);
				break;
			}
		}
		bool operator ()(const itm_t &l, const itm_t &r) const
		{
			return ascending_ ? pr_(r, l) : pr_(l, r);
		}
	};

	struct meterage_formatter_t
	{
		static constexpr auto fill_symbol = '.';
		const std::size_t number_len_{ 1 }; // '#'
		mutable std::size_t n_{ 0 };
		const unsigned flags_;
		const unsigned step_guides_;

		std::size_t max_len_name_{ std::size(TitleName) - 1 };
		std::size_t max_len_total_{ std::size(TitleTotal) - 1 };
		std::size_t max_len_average_{ std::size(TitleAverage) - 1 };
		std::size_t max_len_amount_{ std::size(TitleAmount) - 1 };

		meterage_formatter_t(const std::vector<itm_t> itms, unsigned flags)
		:	number_len_{ itms.empty() ? 1U : 1U + static_cast<std::size_t>(std::floor(std::log10(itms.size()))) },
			flags_{ flags },
			step_guides_{ itms.size() > 4U ? 3U : 0U }
		{
			std::size_t *ptr = &max_len_name_;
			switch (flags_ & vi_tmSortMask)
			{
			case vi_tmSortByAmount:
				ptr = &max_len_amount_;
				break;
			case vi_tmSortByName:
				ptr = &max_len_name_;
				break;
			case vi_tmSortByTime:
				ptr = &max_len_total_;
				break;
			case vi_tmSortBySpeed:
				ptr = &max_len_average_;
				break;
			default:
				assert(false);
				break;
			}
			*ptr += ((flags & vi_tmSortAscending) ? std::size(Ascending): std::size(Descending)) - 1;

			std::size_t max_amount = 0U;
			for (auto &itm : itms)
			{
				max_len_total_ = std::max(max_len_total_, itm.total_txt_.length());
				max_len_average_ = std::max(max_len_average_, itm.average_txt_.length());
				max_len_name_ = std::max(max_len_name_, itm.name_.length());
				if (itm.amount_ > max_amount)
				{
					max_amount = itm.amount_;
					auto len = static_cast<std::size_t>(std::floor(std::log10(max_amount)));
					len += len / 3; // for thousand separators
					len += 1U;
					max_len_amount_ = std::max(max_len_amount_, len);
				}
			}
		}

		int header(const vi_tmLogSTR_t fn, void *const data) const
		{
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
				{
					return std::string{ name } + (sort == s ? order : "");
				};

			std::ostringstream str;
			str <<
				std::setw(number_len_) << "#" << "  " << std::setfill(fill_symbol) << std::left <<
				std::setw(max_len_name_) << title(TitleName, vi_tmSortByName) << ": " << std::setfill(' ') << std::right <<
				std::setw(max_len_average_) << title(TitleAverage, vi_tmSortBySpeed) << " [" <<
				std::setw(max_len_total_) << title(TitleTotal, vi_tmSortByTime) << " / " <<
				std::setw(max_len_amount_) << title(TitleAmount, vi_tmSortByAmount) << "]" <<
				"\n";

			const auto result = str.str();
			assert(number_len_ + 2 + max_len_name_ + 2 + max_len_average_ + 2 + max_len_total_ + 3 + max_len_amount_ + 1 + 1 == result.size());

			return fn(result.c_str(), data);
		}

		int line(const itm_t &i, const vi_tmLogSTR_t fn, void *const data) const
		{
			std::ostringstream str;
			str.imbue(std::locale(str.getloc(), new misc::space_out));

			n_++;
			const char fill = (step_guides_ && (0 == n_ % step_guides_)) ? fill_symbol : ' ';
			str <<
				std::setw(number_len_) << n_ << ". " << std::setfill(fill) << std::left <<
				std::setw(max_len_name_) << i.name_ << ": " << std::setfill(' ') << std::right <<
				std::setw(max_len_average_) << i.average_txt_ << " [" <<
				std::setw(max_len_total_) << i.total_txt_ << " / " <<
				std::setw(max_len_amount_) << i.amount_ << "]" <<
				"\n";

			const auto result = str.str();
			assert(number_len_ + 2 + max_len_name_ + 2 + max_len_average_ + 2 + max_len_total_ + 3 + max_len_amount_ + 1 + 1 == result.size());

			return fn(result.c_str(), data);
		}
	};
}

int VI_TM_CALL vi_tmReport(VI_TM_HANDLE h, unsigned flags, vi_tmLogSTR_t fn, void *data)
{
	props(); // Preventing deadlock in traits_t::results_callback().

	int result = 0;
	if (flags & (vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowResolution))
	{	std::ostringstream str;

		if (flags & vi_tmShowResolution)
		{	str << "Resolution: " << misc::duration_t(props().tick_duration_ * props().clock_resolution_) << ". ";
		}
		if (flags & vi_tmShowDuration)
		{	str << "Duration: " << props().all_latency_ << ". ";
		}
		if (flags & vi_tmShowUnit)
		{	str << "One tick: " << props().tick_duration_ << ". ";
		}
		if (flags & vi_tmShowOverhead)
		{	str << "Additive: " << misc::duration_t(props().tick_duration_ * props().clock_latency_) << ". ";
		}

		str << '\n';
		result = fn(str.str().c_str(), data);
	}

	std::vector<itm_t> itms;
	vi_tmResults(h, &results_callback, &itms);
	std::sort(itms.begin(), itms.end(), meterage_comparator_t{ flags });

	meterage_formatter_t mf{ itms, flags };

	if (0 == (flags & vi_tmShowNoHeader))
	{	result += mf.header(fn, data);
	}

	for (auto itm : itms)
	{ result += mf.line(itm, fn, data);
	}

	return result;
}

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

#include <vi_timing.h>
#include "internal.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <numeric>
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

	constexpr char TitleName[] = { "Name" };
	constexpr char TitleAverage[] = { "Average" };
	constexpr char TitleTotal[] = { "Total" };
	constexpr char TitleAmount[] = { "Amount" };
	constexpr char Ascending [] = { " (^)" };
	constexpr char Descending[] = { " (v)" };
	constexpr char Insignificant[] = { "<insig>" };
	constexpr char NotAvailable[] = { "<n/a>" };

	struct traits_t
	{	struct itm_t;

		std::vector<itm_t> meterages_;
		report_flags_t flags_{};
		std::size_t max_amount_{};
		std::size_t max_len_name_{ std::size(TitleName) - 1};
		std::size_t max_len_total_{ std::size(TitleTotal) - 1};
		std::size_t max_len_average_{ std::size(TitleAverage) - 1};
		std::size_t max_len_amount_{ std::size(TitleAmount) - 1};
		
		explicit traits_t(report_flags_t flags);
		int append(const char *name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt);
		void sort();
		static int callback(const char *name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt, void *data);
	};

	struct traits_t::itm_t
	{	std::string_view orig_name_; // Name
		vi_tmTicks_t orig_total_{}; // Total ticks duration
		std::size_t orig_amount_{}; // Number of measured units
		std::size_t orig_calls_cnt_{}; // To account for overheads

		misc::duration_t total_time_{.0}; // seconds
		misc::duration_t average_{.0}; // seconds
		std::string total_txt_{ NotAvailable };
		std::string average_txt_{ NotAvailable };

		itm_t(const char* n, vi_tmTicks_t t, std::size_t a, std::size_t c) noexcept : orig_name_{ n }, orig_total_{ t }, orig_amount_{ a }, orig_calls_cnt_{ c } {}
	};

	traits_t::traits_t(report_flags_t flags)
		: flags_{ flags }
	{	auto max_len = &max_len_average_;
		switch (flags_ & to_underlying(vi_tmSortMask))
		{
		case vi_tmSortByAmount:
			max_len = &max_len_amount_;
			break;
		case vi_tmSortByName:
			max_len = &max_len_name_;
			break;
		case vi_tmSortByTime:
			max_len = &max_len_total_;
			break;
		default:
			break;
		}
		*max_len += ((flags & to_underlying(vi_tmSortAscending)) ? std::size(Ascending): std::size(Descending)) - 1;
	}

	int traits_t::callback(const char *name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt, void *data)
	{	assert(data);
		return static_cast<traits_t*>(data)->append(name, total, amount, calls_cnt);
	}

	int traits_t::append(const char *name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt)
	{	assert(amount >= calls_cnt);
		assert((0 == calls_cnt) == (0 == amount));
		auto& itm = meterages_.emplace_back(name, total, amount, calls_cnt);

		if (0 == itm.orig_amount_)
		{	assert( 0 == itm.orig_total_);
			assert( 0 == itm.orig_amount_);
			assert( 0 == itm.orig_calls_cnt_);
			assert( misc::duration_t::zero() == itm.total_time_);
			assert( misc::duration_t::zero() == itm.average_);
			assert(itm.total_txt_ == NotAvailable);
			assert(itm.average_txt_ == NotAvailable);
		}
		else if
			(	const auto burden = props().clock_latency_ * itm.orig_calls_cnt_;
				itm.orig_total_ <= burden + props().clock_resolution_ * std::sqrt(itm.orig_calls_cnt_)
			)
		{	itm.total_txt_ = Insignificant;
			itm.average_txt_ = Insignificant;
		}
		else
		{	itm.total_time_ = props().tick_duration_ * (itm.orig_total_ - burden);
			itm.average_ = itm.total_time_ / itm.orig_amount_;
			itm.total_txt_ = to_string(itm.total_time_);
			itm.average_txt_ = to_string(itm.average_);
		}

		max_len_total_ = std::max(max_len_total_, itm.total_txt_.length());
		max_len_average_ = std::max(max_len_average_, itm.average_txt_.length());
		max_len_name_ = std::max(max_len_name_, itm.orig_name_.length());
		if (itm.orig_amount_ > max_amount_)
		{	max_amount_ = itm.orig_amount_;
			auto len = static_cast<std::size_t>(std::floor(std::log10(max_amount_))); //-V203 "Explicit type conversion from memsize to double type or vice versa."
			len += len / 3; // for thousand separators
			len += 1U;
			max_len_amount_ = std::max(max_len_amount_, len);
		}

		return 1; // Continue enumerate.
	}

	template<vi_tmReportFlags_e E> auto make_tuple(const traits_t::itm_t& v);
	template<> auto make_tuple<vi_tmSortByName>(const traits_t::itm_t& v)
	{	return std::tuple{ v.orig_name_ };
	}
	template<> auto make_tuple<vi_tmSortBySpeed>(const traits_t::itm_t& v)
	{	return std::tuple{ v.average_, v.total_time_, v.orig_amount_, v.orig_name_ };
	}
	template<> auto make_tuple<vi_tmSortByTime>(const traits_t::itm_t& v)
	{	return std::tuple{ v.total_time_, v.average_, v.orig_amount_, v.orig_name_ };
	}
	template<> auto make_tuple<vi_tmSortByAmount>(const traits_t::itm_t& v)
	{	return std::tuple{ v.orig_amount_, v.average_, v.total_time_, v.orig_name_ };
	}

	template<vi_tmReportFlags_e E> bool less(const traits_t::itm_t& l, const traits_t::itm_t& r)
	{	return make_tuple<E>(r) < make_tuple<E>(l);
	}

	struct meterage_comparator_t
	{	const report_flags_t flags_{};
		explicit meterage_comparator_t(report_flags_t flags) noexcept
			: flags_{ flags }
		{
		}
		bool operator ()(const traits_t::itm_t& l, const traits_t::itm_t& r) const
		{	auto pr = less<vi_tmSortBySpeed>;
			switch (flags_ & to_underlying(vi_tmSortMask) & ~to_underlying(vi_tmSortAscending))
			{
			case to_underlying(vi_tmSortByName):
				pr = less<vi_tmSortByName>;
				break;
			case to_underlying(vi_tmSortByAmount):
				pr = less<vi_tmSortByAmount>;
				break;
			case to_underlying(vi_tmSortByTime):
				pr = less<vi_tmSortByTime>;
				break;
			case to_underlying(vi_tmSortBySpeed):
				break;
			default:
				assert(false);
				break;
			}

			return (flags_ & to_underlying(vi_tmSortAscending)) ? pr(r, l) : pr(l, r);
		}
	};

	void traits_t::sort()
	{	std::sort(meterages_.begin(), meterages_.end(), meterage_comparator_t{ flags_ });
	}

	struct meterage_formatter_t
	{
		static constexpr auto fill_symbol = '.';
		const traits_t& traits_;
		const vi_tmLogSTR_t fn_;
		void* const data_;
		std::size_t number_len_{1}; // '#'
		mutable std::size_t n_{ 0 };

		meterage_formatter_t(const traits_t& traits, vi_tmLogSTR_t fn, void* data)
		:	traits_{ traits }, fn_{ fn }, data_{ data }
		{	if (auto size = traits_.meterages_.size(); 0 != size)
			{	number_len_ = 1U + static_cast<std::size_t>(std::floor(std::log10(size))); //-V203 "Explicit type conversion from memsize to double type or vice versa."
			}
		}

		int header() const
		{	auto sort = vi_tmSortBySpeed;
			switch (auto s = traits_.flags_ & to_underlying(vi_tmSortMask) & ~to_underlying(vi_tmSortAscending))
			{
			case vi_tmSortBySpeed:
				break;
			case vi_tmSortByAmount:
			case vi_tmSortByName:
			case vi_tmSortByTime:
				sort = static_cast<vi_tmReportFlags_e>(s);
				break;
			default:
				assert(false);
				break;
			}

			auto title = [sort, order = (traits_.flags_ & to_underlying(vi_tmSortAscending)) ? Ascending : Descending]
				(const char *name, vi_tmReportFlags_e s)
				{	return std::string{ name } + (sort == s ? order : "");
				};

			std::ostringstream str;
			str << 
				std::setw(number_len_) << "#" << "  " <<
				std::setw(traits_.max_len_name_) << std::setfill(fill_symbol) << std::left << title(TitleName, vi_tmSortByName) << ": " <<
				std::setw(traits_.max_len_average_) << std::setfill(' ') << std::right << title(TitleAverage, vi_tmSortBySpeed) << " [" <<
				std::setw(traits_.max_len_total_) << title(TitleTotal, vi_tmSortByTime) << " / " <<
				std::setw(traits_.max_len_amount_) << title(TitleAmount, vi_tmSortByAmount) << "]" <<
				"\n";

			const auto result = str.str();
			assert(number_len_ + 2 + traits_.max_len_name_ + 2 + traits_.max_len_average_ + 2 + traits_.max_len_total_ + 3 + traits_.max_len_amount_ + 1 + 1 == result.size());

			return fn_(result.c_str(), data_);
		}

		int operator ()(int init, const traits_t::itm_t& i) const
		{	std::ostringstream str;
			str.imbue(std::locale(str.getloc(), new misc::space_out));

			constexpr auto step_guides = 3;
			n_++;
			const char fill = (traits_.meterages_.size() > step_guides + 1 && (0 == n_ % step_guides)) ? fill_symbol : ' ';
			str << 
				std::setw(number_len_) << n_ << ". " <<
				std::setw(traits_.max_len_name_) << std::setfill(fill) << std::left << i.orig_name_ << ": " <<
				std::setw(traits_.max_len_average_) << std::setfill(' ') << std::right << i.average_txt_ << " [" <<
				std::setw(traits_.max_len_total_) << i.total_txt_ << " / " <<
				std::setw(traits_.max_len_amount_) << i.orig_amount_ << "]" <<
				"\n";

			auto result = str.str();
			assert(number_len_ + 2 + traits_.max_len_name_ + 2 + traits_.max_len_average_ + 2 + traits_.max_len_total_ + 3 + traits_.max_len_amount_ + 1 + 1 == result.size());

			return init + fn_(result.c_str(), data_);
		}
	};
} // namespace {

int VI_TM_CALL vi_tmReport(VI_TM_HANDLE h, int flagsa, vi_tmLogSTR_t fn, void* data)
{	report_flags_t flags = 0;
	static_assert(sizeof(flags) == sizeof(flagsa));
	std::memcpy(&flags, &flagsa, sizeof(flags));

	props(); // Preventing deadlock in traits_t::callback().

	traits_t traits{ flags };
	vi_tmResults(h, traits_t::callback, &traits);

	traits.sort();

	int ret = 0;
	if (flags & (vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit))
	{	std::ostringstream str;

		if (flags & to_underlying(vi_tmShowResolution))
		{	str << "Resolution: " << misc::duration_t(props().tick_duration_ * props().clock_resolution_) << ". ";
		}
		if (flags & to_underlying(vi_tmShowDuration))
		{	str << "Duration: " << props().all_latency_ << ". ";
		}
		if (flags & to_underlying(vi_tmShowUnit))
		{	str << "One tick: " << props().tick_duration_ << ". ";
		}
		if (flags & to_underlying(vi_tmShowOverhead))
		{	str << "Additive: " << misc::duration_t(props().tick_duration_ * props().clock_latency_) << ". ";
		}

		str << '\n';
		ret = fn(str.str().c_str(), data);
	}

	meterage_formatter_t mf{ traits, fn, data };
	if (0 == (flags & to_underlying(vi_tmShowNoHeader)))
	{	ret += mf.header();
	}
	return std::accumulate(traits.meterages_.begin(), traits.meterages_.end(), ret, mf);
}

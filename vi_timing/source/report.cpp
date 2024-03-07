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
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

namespace ch = std::chrono;
using namespace std::literals;

namespace
{
	constexpr auto operator""_ps(long double v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ps(unsigned long long v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ks(long double v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_ks(unsigned long long v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_Ms(long double v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Ms(unsigned long long v) noexcept { return ch::duration<double, std::mega>(v); };

	struct duration_t : ch::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		using ch::duration<double>::duration;
		template<typename T>
		constexpr explicit duration_t(T&& v) : ch::duration<double>{ std::forward<T>(v) } {}

		friend std::string to_string(duration_t sec, unsigned char precision = 2, unsigned char dec = 1);

		friend [[nodiscard]] bool operator<(duration_t l, duration_t r)
		{	return l.count() < r.count() && to_string(l) != to_string(r);
		}
	};

	inline std::ostream& operator<<(std::ostream& os, const duration_t& d)
	{	return os << to_string(d);
	}

	[[nodiscard]] double round(double num, unsigned char prec, unsigned char dec = 1)
	{ // Rounding to 'dec' decimal place and no more than 'prec' significant symbols.
		double result;
		if (num >= 5e-12 && prec != 0 && prec > dec)
		{
			const auto exp = static_cast<signed char>(std::ceil(std::log10(num)));
			if (const auto n = 1 + dec + (11 + exp) % 3; prec > n)
			{
				prec = static_cast<unsigned char>(n);
			}

			const auto factor = std::max(10e-12, std::pow(10.0, exp - prec)); // The lower limit of accuracy is 0.01ns.
			result = std::round((num * (1 + std::numeric_limits<decltype(num)>::epsilon())) / factor) * factor;
		}
		else
		{
			assert(num < 5e-12);
			result = num;
		}
		return result;
	}

	[[nodiscard]] std::string to_string( duration_t sec, unsigned char precision, unsigned char dec)
	{
		sec = duration_t{ round(sec.count(), precision, dec) };

		std::pair<std::string_view, double> k;
		if (10_ps > sec)			{ k = {"ps", 1.0}; }
		else if (999.95_ps > sec)	{ k = {"ps", 1e12}; }
		else if (999.95ns > sec)	{ k = {"ns", 1e9}; }
		else if (999.95us > sec)	{ k = {"us", 1e6}; }
		else if (999.95ms > sec)	{ k = {"ms", 1e3}; }
		else if (999.95s > sec)		{ k = {"s ", 1e0}; }
		else if (999.95_ks > sec)	{ k = {"ks", 1e-3}; }
		else if (999.95_Ms > sec)	{ k = {"Ms", 1e-6}; }
		else						{ k = {"Gs", 1e-9}; }

		std::ostringstream ss;
		ss << std::fixed << std::setprecision(dec) << sec.count() * k.second << k.first;
		return ss.str();
	}

#ifndef NDEBUG
	const auto unit_test_to_string = []
	{
		struct I { duration_t sec_; std::string_view res_; unsigned char precision_{ 2 }; unsigned char dec_{ 1 }; };
		static constexpr I samples[] = {
			{10.1ms, "10.0ms"},
			{10.01ms, "10.0ms"},
			{1234567.89s, "1.2Ms", 9, 1},
			{123456.789s, "123.457ks", 9, 3},
			{0, "0.0ps"},
			{0.1_ps, "0.0ps"},
			{1_ps, "0.0ps"}, // The lower limit of accuracy is 10ps.
			{10_ps, "10.0ps"},
			{100_ps, "100.0ps"},
			{1ns, "1.0ns"},
			{10ns, "10.0ns"},
			{100ns, "100.0ns"},
			{1us, "1.0us"},
			{10us, "10.0us"},
			{100us, "100.0us"},
			{1ms, "1.0ms"},
			{10ms, "10.0ms"},
			{100ms, "100.0ms"},
			{1s, "1.0s "},
			{10s, "10.0s "},
			{100s, "100.0s "},
			{1min, "60.0s "},
			{1h, "3.6ks"},
			{4.999999999999_ps, "0ps", 1, 0},
			{4.999999999999_ps, "0.0ps", 2},
			{4.999999999999_ps, "0.0ps", 3},
			{4.999999999999_ps, "0.0ps", 4},
			{5.000000000000_ps, "10ps", 1, 0},
			{5.000000000000_ps, "10.0ps", 2},
			{5.000000000000_ps, "10.0ps", 3},
			{5.000000000000_ps, "10.0ps", 4},
			{4.499999999999ns, "4ns", 1, 0},
			{4.499999999999ns, "4.5ns", 2},
			{4.499999999999ns, "4.5ns", 3},
			{4.499999999999ns, "4.5ns", 4},
			{4.999999999999ns, "5ns", 1, 0},
			{4.999999999999ns, "5.0ns", 2},
			{4.999999999999ns, "5.0ns", 3},
			{4.999999999999ns, "5.0ns", 4},
			{5.000000000000ns, "5ns", 1, 0},
			{5.000000000000ns, "5.0ns", 2},
			{5.000000000000ns, "5.0ns", 3},
			{5.000000000000ns, "5.0ns", 4},
			{123.4ns, "100ns", 1, 0},
			{123.4ns, "120.0ns", 2},
			{123.4ns, "123.0ns", 3},
			{123.4ns, "123.4ns", 4},
			{4.999999999999_ps, "0.0ps", 2, 1},
			{4.999999999999_ps, "0.00ps", 3, 2},
			{4.999999999999_ps, "0.00ps", 4, 2},
			{5.000000000000_ps, "10.00ps", 3, 2},
			{5.000000000000_ps, "10.00ps", 4, 2},
			{4.499999999999ns, "4.5ns", 2, 1},
			{4.499999999999ns, "4.50ns", 3, 2},
			{4.499999999999ns, "4.50ns", 4, 2},
			{4.999999999999ns, "5.0ns", 2, 1},
			{4.999999999999ns, "5.00ns", 3, 2},
			{4.999999999999ns, "5.00ns", 4, 2},
			{5.000000000000ns, "5.0ns", 2, 1},
			{5.000000000000ns, "5.00ns", 3, 2},
			{5.000000000000ns, "5.00ns", 4, 2},
			{123.4ns, "120.0ns", 2, 1},
			{123.4ns, "123.00ns", 3, 2},
			{123.4ns, "123.40ns", 4, 2},
			//**********************************
			{0.0_ps, "0.0ps"},
			{0.123456789us, "123.5ns", 4},
			{1.23456789s, "1s ", 1, 0},
			{1.23456789s, "1.2s ", 3},
			{1.23456789s, "1.2s "},
			{1.23456789us, "1.2us"},
			{1004.4ns, "1.0us", 2},
			{12.3456789s, "10s ", 1, 0},
			{12.3456789s, "12.3s ", 3},
			{12.3456789us, "12.3us", 3},
			{12.3456s, "12.0s "},
			{12.34999999ms, "10ms", 1, 0},
			{12.34999999ms, "12.3ms", 3},
			{12.34999999ms, "12.3ms", 4},
			{12.4999999ms, "12.0ms"},
			{12.4999999ms, "12.5ms", 3},
			{12.5000000ms, "13.0ms"},
			{123.456789ms, "123.0ms", 3},
			{123.456789us, "120.0us"},
			{123.4999999ms, "123.5ms", 4},
			{1234.56789us, "1.2ms"},
			{245.0_ps, "250.0ps"},
			{49.999_ps, "50.0ps"},
			{50.0_ps, "50.0ps"},
			{9.49999_ps, "10.0ps"},
			{9.9999_ps, "10.0ps"}, // The lower limit of accuracy is 10ps.
			{9.999ns, "10.0ns"},
			{99.49999_ps, "100.0ps"},
			{99.4999ns, "99.0ns"},
			{99.4ms, "99.0ms"},
			{99.5_ps, "100.0ps"},
			{99.5ms, "100.0ms"},
			{99.5ns, "100.0ns"},
			{99.5us, "100.0us"},
			{99.999_ps, "100.0ps"},
			{999.0_ps, "1.0ns"},
			{999.45ns, "1us", 1, 0},
			{999.45ns, "1.0us", 2},
			{999.45ns, "999.0ns", 3},
			{999.45ns, "999.5ns", 4},
			{999.45ns, "999.45ns", 5, 2},
			{999.55ns, "1.0us", 3},
			{99ms, "99.0ms"},
		};

		for (auto& i : samples)
		{	const auto str = to_string(i.sec_, i.precision_, i.dec_);
			assert(i.res_ == str);
		}

		return 0;
	}();
#endif // #ifndef NDEBUG

	duration_t seconds_per_tick()
	{
		auto wait_for_the_time_changed = []
		{
			{
				std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
				[[maybe_unused]] volatile auto dummy_1 = vi_tmGetTicks(); // Preloading a function into cache
				[[maybe_unused]] volatile auto dummy_2 = ch::steady_clock::now(); // Preloading a function into cache
			}

			// Are waiting for the start of a new time interval.
			auto last = ch::steady_clock::now();
			for (const auto s = last; s == last; last = ch::steady_clock::now())
			{/**/}

			return std::tuple{ vi_tmGetTicks(), last };
		};

		const auto [tick1, time1] = wait_for_the_time_changed();
		// Load the thread at 100% for 256ms.
		for (auto stop = time1 + 256ms; ch::steady_clock::now() < stop;)
		{/**/}
		const auto [tick2, time2] = wait_for_the_time_changed();

		return duration_t(time2 - time1) / (tick2 - tick1);
	}

	duration_t duration()
	{
		static constexpr auto CNT = 1'000U;

		auto foo = [] {
			// The order of calling the functions is deliberately broken. To push 'vi_tmGetTicks()' and 'vi_tmAdd()' further apart.
			const auto s = vi_tmGetTicks();
			auto itm = vi_tmItem("", 1);
			vi_tmAdd(itm, s);
		};

		auto start = [] {
			std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			// Are waiting for the start of a new time interval.
			auto last = ch::steady_clock::now();
			for (const auto s = last; s == last; last = ch::steady_clock::now())
			{/**/}
			return last;
		};

		foo(); // Create a service item with empty name "".

		auto b = start();
		for (size_t cnt = CNT; cnt; --cnt)
		{ 
			foo();
		}
		const auto pure = ch::steady_clock::now() - b;

		b = start();
		static constexpr auto EXT = 10U;
		for (size_t cnt = CNT; cnt; --cnt)
		{ 
			foo();

			// EXT calls
			foo(); foo(); foo(); foo(); foo();
			foo(); foo(); foo(); foo(); foo();
		}
		const auto dirty = ch::steady_clock::now() - b;

		return duration_t(dirty - pure) / (EXT * CNT);
	}

VI_OPTIMIZE_OFF
	double measurement_cost()
	{
		constexpr auto CNT = 1'000U;

		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		auto e = vi_tmGetTicks(); // Preloading a function into cache
		auto s = vi_tmGetTicks();
		for (auto cnt = CNT; cnt; --cnt)
		{
			e = vi_tmGetTicks();
		}
		const auto pure = e - s;

		constexpr auto EXT = 30U;
		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		e = vi_tmGetTicks(); // Preloading a function into cache
		s = vi_tmGetTicks();
		for (auto cnt = CNT; cnt; --cnt)
		{
			e = vi_tmGetTicks(); //-V761

			// EXT calls
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
		}
		const auto dirty = e - s;

		return static_cast<double>(dirty - pure) / (EXT * CNT);
	}
VI_OPTIMIZE_ON

	constexpr std::string_view title_name_ = "Name";
	constexpr std::string_view title_average_ = "Average";
	constexpr std::string_view title_total_ = "Total";
	constexpr std::string_view title_amount_ = "Amount";

	struct traits_t
	{
		struct itm_t
		{
			std::string_view on_name_; // Name
			vi_tmTicks_t on_total_{}; // Total ticks duration
			std::size_t on_amount_{}; // Number of measured units
			std::size_t on_calls_cnt_{}; // To account for overheads

			duration_t total_time_{}; // seconds
			duration_t average_{}; // seconds
			std::string total_txt_;
			std::string average_txt_;
			itm_t(const char* n, vi_tmTicks_t t, std::size_t a, std::size_t c) noexcept : on_name_{ n }, on_total_{ t }, on_amount_{ a }, on_calls_cnt_{ c } {}
		};

		std::vector<itm_t> meterages_;

		const duration_t tick_duration_ = seconds_per_tick();
		const double measurement_cost_ = measurement_cost(); // ticks
		std::size_t max_amount_{};
		std::size_t max_len_name_{ title_name_.length()};
		std::size_t max_len_total_{ title_total_.length() };
		std::size_t max_len_average_{ title_average_.length() };
		std::size_t max_len_amount_{ title_amount_.length() };
	};

	int collector_meterages(const char* name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt, void* _traits)
	{
		assert(_traits);
		auto& traits = *static_cast<traits_t*>(_traits);
		assert(calls_cnt && amount >= calls_cnt);

		auto& itm = traits.meterages_.emplace_back(name, total, amount, calls_cnt);

		constexpr auto dirty = 1.1; // A fair odds would be 1.
		if (const auto total_over_ticks = traits.measurement_cost_ * itm.on_calls_cnt_ / dirty; itm.on_total_ > total_over_ticks)
		{
			itm.total_time_ = traits.tick_duration_ * (itm.on_total_ - total_over_ticks);
			itm.average_ = itm.total_time_ / itm.on_amount_;
			itm.average_txt_ = to_string(itm.average_);
		}
		else
		{
			// Leave zeros.
			itm.total_time_ = duration_t{};
			itm.average_ = duration_t{};
			itm.average_txt_ = "<too few>";
		}

		itm.total_txt_ = to_string(itm.total_time_);
		traits.max_len_total_ = std::max(traits.max_len_total_, itm.total_txt_.length());
		traits.max_len_average_ = std::max(traits.max_len_average_, itm.average_txt_.length());
		traits.max_len_name_ = std::max(traits.max_len_name_, itm.on_name_.length());

		if (itm.on_amount_ > traits.max_amount_)
		{
			traits.max_amount_ = itm.on_amount_;
			auto max_len_amount = static_cast<std::size_t>(std::floor(std::log10(itm.on_amount_)));
			max_len_amount += max_len_amount / 3; // for thousand separators
			max_len_amount += 1;
			traits.max_len_amount_ = max_len_amount;
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
				result = desc ? (l.on_name_ > r.on_name_) : (l.on_name_ < r.on_name_);
				break;
			case static_cast<uint32_t>(vi_tmSortByAmount):
				result = desc ? (l.on_amount_ > r.on_amount_) : (l.on_amount_ < r.on_amount_);
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
		const vi_tmLogSTR_t fn_;
		void* const data_;
		std::size_t number_len_{0};
		mutable std::size_t n_{ 0 };

		meterage_format_t(traits_t& traits, vi_tmLogSTR_t fn, void* data)
			:traits_{ traits }, fn_{ fn }, data_{ data }
		{
			if (auto size = traits_.meterages_.size(); size >= 1)
			{
				number_len_ = 1 + static_cast<int>(std::floor(std::log10(size)));
			}
		}

		int header() const
		{
			std::ostringstream str;
			str << std::setw(number_len_) << "#" << "  ";
			str << std::setw(traits_.max_len_name_) << std::left << title_name_ << ": ";
			str << std::setw(traits_.max_len_average_) << std::setfill(' ') << std::right << title_average_ << " [";
			str << std::setw(traits_.max_len_total_) << title_total_ << " / ";
			str << std::setw(traits_.max_len_amount_) << title_amount_ << "]";
			str << "\n";

			auto result = str.str();
			assert(number_len_ + 2 + traits_.max_len_name_ + 2 + traits_.max_len_average_ + 2 + traits_.max_len_total_ + 3 + traits_.max_len_amount_ + 1 + 1 == result.size());

			return fn_(result.c_str(), data_);
		}

		int operator ()(int init, const traits_t::itm_t& i) const
		{
			n_++;

			std::ostringstream str;
			struct thousands_sep_facet_t final : std::numpunct<char>
			{
				char do_thousands_sep() const override { return '\''; }
				std::string do_grouping() const override { return "\x3"; }
			};
			str.imbue(std::locale(str.getloc(), new thousands_sep_facet_t)); //-V2511

			const char fill = (traits_.meterages_.size() > 4 && (n_ - 1) % 2) ? ' ' : '.';
			str << std::setw(number_len_) << n_ << ". ";
			str << std::setw(traits_.max_len_name_) << std::setfill(fill) << std::left << i.on_name_ << ": ";
			str << std::setw(traits_.max_len_average_) << std::setfill(' ') << std::right << i.average_txt_ << " [";
			str << std::setw(traits_.max_len_total_) << i.total_txt_ << " / ";
			str << std::setw(traits_.max_len_amount_) << i.on_amount_ << "]";
			str << "\n";

			auto result = str.str();
			assert(number_len_ + 2 + traits_.max_len_name_ + 2 + traits_.max_len_average_ + 2 + traits_.max_len_total_ + 3 + traits_.max_len_amount_ + 1 + 1 == result.size());

			return init + fn_(result.c_str(), data_);
		}
	};
} // namespace {

VI_TM_API int VI_TM_CALL vi_tmReport(vi_tmLogSTR_t fn, void* data, std::uint32_t flags)
{
	traits_t traits;
	vi_tmResults(collector_meterages, &traits);

	std::sort(traits.meterages_.begin(), traits.meterages_.end(), meterage_comparator_t{ flags });

	int ret = 0;
	std::ostringstream str;
	if (flags & static_cast<uint32_t>(vi_tmShowOverhead))
	{
		str << "Measurement cost: " << traits.tick_duration_ * traits.measurement_cost_ << " per measurement. ";
	}

	if (flags & static_cast<uint32_t>(vi_tmShowDuration))
	{
		str << "Duration: " << duration() << ". ";
	}

	if (flags & static_cast<uint32_t>(vi_tmShowUnit))
	{
		str << "One tick corresponds: " << traits.tick_duration_ << ". ";
	}

	str << '\n';
	auto s = str.str();
	ret += fn(s.c_str(), data);
	meterage_format_t mf{ traits, fn, data };
	ret += mf.header();
	return std::accumulate(traits.meterages_.begin(), traits.meterages_.end(), ret, mf);
}

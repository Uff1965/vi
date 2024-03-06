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

namespace
{
	namespace ch = std::chrono;
	using namespace std::literals;

	constexpr auto operator""_ps(long double v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ps(unsigned long long v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ks(long double v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_ks(unsigned long long v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_Ms(long double v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Ms(unsigned long long v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Gs(long double v) noexcept { return ch::duration<double, std::giga>(v); };
	constexpr auto operator""_Gs(unsigned long long v) noexcept { return ch::duration<double, std::giga>(v); };

	struct duration_t : ch::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		using base_t = ch::duration<double>;
		constexpr explicit duration_t(const base_t& r) : base_t{ r } {}
		using base_t::base_t;

		template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
		friend constexpr [[nodiscard]] duration_t operator*(const duration_t& d, T f)
		{
			return duration_t{ d.count() * f };
		}

		template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
		friend constexpr [[nodiscard]] duration_t operator*(T f, const duration_t& d)
		{
			return d * f;
		}

		friend constexpr double [[nodiscard]] operator/(const duration_t& l, const duration_t& r)
		{
			return l.count() / r.count();
		}

		template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
		friend constexpr [[nodiscard]] duration_t operator/(const duration_t& d, T f)
		{
			return duration_t{ d.count() / f };
		}
	};

	[[nodiscard]] std::string to_string(duration_t sec, unsigned char precision = 2, unsigned char dec = 1);

	inline [[nodiscard]] bool operator<(duration_t l, duration_t r)
	{
		return l.count() < r.count() && to_string(l) != to_string(r);
	}

	inline std::ostream& operator<<(std::ostream& os, const duration_t& d)
	{
		return os << to_string(d);
	}

	double round(double num, unsigned char prec, unsigned char dec = 1)
	{ // Rounding to 'dec' decimal place and no more than 'prec' significant symbols.
		if (num < 5e-12 || prec == 0 || prec <= dec) {
			assert(num < 5e-12);
			return num;
		}

		const auto exp = static_cast<signed char>(std::floor(std::log10(num)));
		if (const auto n = 1 + dec + (12 + exp) % 3; prec > n)
		{
			prec = static_cast<unsigned char>(n);
		}

		const auto factor = std::max(10e-12, std::pow(10.0, exp - (prec - 1))); // The lower limit of accuracy is 0.01ns.
		return std::round((num * (1 + std::numeric_limits<decltype(num)>::epsilon())) / factor) * factor;
	}

	std::string to_string( duration_t sec, unsigned char precision, unsigned char dec)
	{
		sec = duration_t{ round(sec.count(), precision, dec) };

		auto prn = [sec, dec](const char* u, double e) {
			std::stringstream ss;
			ss << std::fixed << std::setprecision(dec) << sec.count() * e << u;
			return ss.str();
			};

		std::string result;
		if (10_ps > sec)			{ result = prn("ps", 1.0); }
		else if (999.95_ps > sec)	{ result = prn("ps", 1e12); }
		else if (999.95ns > sec)	{ result = prn("ns", 1e9); }
		else if (999.95us > sec)	{ result = prn("us", 1e6); }
		else if (999.95ms > sec)	{ result = prn("ms", 1e3); }
		else if (999.95s > sec)		{ result = prn("s ", 1e0); }
		else if (999.95_ks > sec)	{ result = prn("ks", 1e-3); }
		else if (999.95_Ms > sec)	{ result = prn("Ms", 1e-6); }
		else						{ result = prn("Gs", 1e-9); }

		return result;
	}

#ifndef NDEBUG
	const auto unit_test_to_string = []
		{
			struct I { duration_t sec_; std::string_view res_; unsigned char precision_{ 2 }; unsigned char dec_{ 1 }; };
			static constexpr I samples[] = {
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

				{4.999999999999_ps, "0ps", 1, 0},
				{4.999999999999_ps, "0.0ps", 2, 1},
				{4.999999999999_ps, "0.00ps", 3, 2},
				{4.999999999999_ps, "0.00ps", 4, 2},
				{5.000000000000_ps, "10.00ps", 3, 2},
				{5.000000000000_ps, "10.00ps", 4, 2},

				{4.499999999999ns, "4.5ns", 2, 1},
				{4.499999999999ns, "4.50ns", 3, 2},
				{4.499999999999ns, "4.50ns", 4, 2},
				{4.999999999999ns, "5ns", 1, 0},
				{4.999999999999ns, "5.0ns", 2, 1},
				{4.999999999999ns, "5.00ns", 3, 2},
				{4.999999999999ns, "5.00ns", 4, 2},
				{5.000000000000ns, "5ns", 1, 0},
				{5.000000000000ns, "5.0ns", 2, 1},
				{5.000000000000ns, "5.00ns", 3, 2},
				{5.000000000000ns, "5.00ns", 4, 2},

				{123.4ns, "100ns", 1, 0},
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
				{1s, "1.0s "},
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
			{
				const auto str = to_string(i.sec_, i.precision_, i.dec_);
				assert(i.res_ == str);
			}

			return 0;
		}();
#endif // #ifndef NDEBUG

VI_OPTIMIZE_OFF
	duration_t seconds_per_tick()
	{
		auto wait_for_the_time_to_change = []
		{
			std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			[[maybe_unused]] volatile auto dummy_1 = vi_tmGetTicks(); // Preloading a function into cache
			[[maybe_unused]] volatile auto dummy_2 = ch::steady_clock::now(); // Preloading a function into cache

			auto last = ch::steady_clock::now();
			for (const auto s = last; s == last; last = ch::steady_clock::now())
			{/**/}

			return std::tuple{ vi_tmGetTicks(), last };
		};

		const auto [tick1, time1] = wait_for_the_time_to_change();
		// Load the thread at 100% for 'interval'.
		constexpr auto interval = 256ms;
		for (auto stop = time1 + interval; ch::steady_clock::now() < stop;)
		{/**/}
		const auto [tick2, time2] = wait_for_the_time_to_change();

		return duration_t(time2 - time1) / (tick2 - tick1);
	}

	duration_t duration()
	{
		static constexpr auto CNT = 10'000U;
		static constexpr auto EXT = 10U;

		static auto foo = [] {
			auto itm = vi_tmItem("", 1);
			const auto s = vi_tmGetTicks();
			vi_tmAdd(itm, s);
		};

		auto pure = [] {
			std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.

			auto b = ch::steady_clock::now();
			for (const auto s = b; s == b; b = ch::steady_clock::now())
			{/**/}

			for (size_t cnt = CNT; cnt; --cnt)
			{ 
				foo();
			}
			return ch::steady_clock::now() - b;
		};

		auto dirty = [] {
			std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.

			auto b = ch::steady_clock::now();
			for (const auto s = b; s == b; b = ch::steady_clock::now())
			{/**/}

			for (size_t cnt = CNT; cnt; --cnt)
			{ 
				foo();

				// EXT calls
				foo(); foo(); foo(); foo(); foo();
				foo(); foo(); foo(); foo(); foo();
			}
			return ch::steady_clock::now() - b;
		};

		return duration_t(dirty() - pure()) / (EXT * CNT);
	}

	double measurement_cost()
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
				e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
				e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			}
			return e - s;
		};

		return static_cast<double>(dirty() - pure()) / (EXT * CNT);
	}
VI_OPTIMIZE_ON

	struct traits_t
	{
		struct itm_t
		{
			std::string_view name_; // Name
			vi_tmTicks_t total_{}; // Total ticks duration
			std::size_t amount_{}; // Number of measured units
			std::size_t calls_cnt_{}; // To account for overheads

			duration_t total_time_{}; // seconds
			duration_t average_{}; // seconds
			std::string total_txt_;
			std::string average_txt_;
			itm_t(const char* n, vi_tmTicks_t t, std::size_t a, std::size_t c) noexcept : name_{ n }, total_{ t }, amount_{ a }, calls_cnt_{ c } {}
		};

		std::vector<itm_t> meterages_;

		const duration_t tick_duration_ = seconds_per_tick();
		const duration_t duration_ = duration(); // duration one measerement. [sec].
		const double measurement_cost_ = measurement_cost(); // ticks
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

		v.max_len_name_ = std::max(v.max_len_name_, itm.name_.length());

		if (const auto total_over_ticks = v.measurement_cost_ * itm.calls_cnt_; itm.total_ > total_over_ticks)
		{
			itm.total_time_ = (itm.total_ - total_over_ticks) * v.tick_duration_;
			itm.average_ = itm.total_time_ / itm.amount_;
		}
		else
		{
			// Leave zeros.
		}

		itm.total_txt_ = to_string(itm.total_time_);
		v.max_len_total_ = std::max(v.max_len_total_, itm.total_txt_.length());

		itm.average_txt_ = to_string(itm.average_);
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
				result = desc ? (l.name_ > r.name_) : (l.name_ < r.name_);
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

			constexpr auto clearance = 1;
			str << std::setw(traits_.max_len_name_) << std::setfill(n_++ % clearance ? ' ' : '.') << std::left << i.name_ << ": ";
			str << std::setw(traits_.max_len_average_) << std::setfill(' ') << std::right << i.average_txt_ << " [";
			str << std::setw(traits_.max_len_total_) << i.total_txt_ << " / ";
			str << std::setw(traits_.max_len_amount_) << i.amount_ << "]";
			str << "\n";

			auto result = str.str();
			assert(traits_.max_len_name_ + 2 + traits_.max_len_average_ + 2 + traits_.max_len_total_ + 3 + traits_.max_len_amount_ + 1 + 1 == result.size());

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
	{
		str << "Measurement cost: " << traits.tick_duration_ * traits.measurement_cost_ << " per measurement. ";
	}

	if (flags & static_cast<uint32_t>(vi_tmShowDuration))
	{
		str << "Duration: " << traits.duration_ << ". ";
	}

	if (flags & static_cast<uint32_t>(vi_tmShowUnit))
	{
		str << "One tick corresponds: " << traits.tick_duration_ << ". ";
	}

	str << '\n';
	auto s = str.str();
	ret += fn(s.c_str(), data);

	std::size_t n = 0;
	return std::accumulate(traits.meterages_.begin(), traits.meterages_.end(), ret, meterage_format_t{ n, traits, fn, data });
}

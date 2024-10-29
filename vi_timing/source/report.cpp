// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/********************************************************************\
'vi_timing' is a small library for measuring the time execution of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was created to experiment for educational purposes.
Do not expect much from it. If you spot a bug or can suggest any
improvement to the code, please email me at eMail:programmer.amateur@proton.me.

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

//-V:assert:2570

#include <timing.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cfloat>
#include <chrono>
#include <climits>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string_view>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ch = std::chrono;
using namespace std::literals;

namespace
{
	struct space_out: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	constexpr auto operator""_ps(long double v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ps(unsigned long long v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ks(long double v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_ks(unsigned long long v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_Ms(long double v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Ms(unsigned long long v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Gs(long double v) noexcept { return ch::duration<double, std::giga>(v); };
	constexpr auto operator""_Gs(unsigned long long v) noexcept { return ch::duration<double, std::giga>(v); };

	[[nodiscard]] double round_triple(double num, int prec = 2, int dec = 1, int group = 3)
	{ // Rounding to 'dec' decimal place and no more than 'prec' significant symbols.
		constexpr auto EPS = std::numeric_limits<decltype(num)>::epsilon();
		assert(num >= .0 && dec >= 0 && group > 0 && prec > dec && group > dec);
		if (num <= .0 || dec < 0 || group <= 0 || prec <= dec)
		{	return num;
		}
		if (dec >= group)
		{	dec %= group;
		}

		const auto exp = 1 + static_cast<int>(std::floor(std::log10(num)));
		assert(0.1 * (1.0 - EPS) <= num * std::pow(10, -exp) && num * std::pow(10, -exp) < 1.0 + EPS);

		auto mod_norm = [group](int n) { auto r = n % group; if (r <= 0) { r += group; } return r; };
		if (auto num_ext = mod_norm(exp); num_ext < prec)
		{	if (const auto prec_ext = (prec - num_ext) % group; prec_ext > dec)
			{	prec -= prec_ext - dec;
			}
		}

		const auto factor = std::pow(10, prec - exp);
		return std::round(factor * (num * (1 + EPS))) / factor;
	}

	struct duration_t : ch::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
//		using ch::duration<double>::duration;
		template<typename T>
		constexpr duration_t(T&& v) : ch::duration<double>{ std::forward<T>(v) } {}

		[[nodiscard]] friend std::string to_string(duration_t sec, unsigned char precision = 2, unsigned char dec = 1)
		{	sec = round_triple(sec.count(), precision, dec);

			dec += 3 * ((precision - 1 - dec) / 3);
			std::ostringstream ss;
			ss << std::fixed << std::setprecision(dec);
			
			struct { std::string_view suffix_; double factor_; } k;
				
			if ( 10_ps > sec)
			{	sec = 0.0;
				k = { "ps"sv, 1e12 };
			}
			else if (1ns  > sec) { k = { "ps"sv, 1e12 }; }
			else if (1us  > sec) { k = { "ns"sv, 1e9 }; }
			else if (1ms  > sec) { k = { "us"sv, 1e6 }; }
			else if (1s   > sec) { k = { "ms"sv, 1e3 }; }
			else if (1_ks > sec) { k = { "s "sv, 1e0 }; }
			else if (1_Ms > sec) { k = { "ks"sv, 1e-3 }; }
			else if (1_Gs > sec) { k = { "Ms"sv, 1e-6 }; }
			else { k = { "Gs"sv, 1e-9 }; }

			ss << sec.count() * k.factor_ << k.suffix_;
			return ss.str();
		}

		[[nodiscard]] friend bool operator<(duration_t l, duration_t r)
		{	return l.count() < r.count() && to_string(l, 2, 1) != to_string(r, 2, 1);
		}
	};


#ifndef NDEBUG
	const auto unit_test_round= []
	{	static constexpr struct
		{	int line_;
			double original_;
			double expected_;
			unsigned char precision_ = 2;
			unsigned char dec_ = 1;
		} //-V802 //-V730
		samples[] = {
#	define ITM(v, ...) {__LINE__, (v), __VA_ARGS__}
			ITM(5.555'555'5, 5.556, 4, 0),
			ITM(5.555'555'5, 5.556, 4, 1),
			ITM(5.555'555'5, 5.556, 4, 2),

			ITM(12.345'678'912,	12.345'679, 8, 2),

			ITM(12.345'678'912,	12.346'000, 7, 0),
			ITM(12.345'678'912,	12.345'680, 7, 2),

			ITM(12.345'678'912,	12.345'700, 6, 2),
			ITM(12.345'678'912,	12.346'000, 5, 2),
			ITM(12.345'678'912,	12.350'000, 4, 2),

			ITM(12.345'678'912,	12.345'679, 8, 0),
			ITM(12.345'678'912,	12.346'000, 7, 0),
			ITM(12.345'678'912,	12.345'678'900, 9, 1),

			ITM(1.111'111'1, 1.111'11, 6, 2),
			ITM(12'345.678'9, 12'345.679, 8, 2),
			ITM(12'345.678'9, 12'345.679, 8, 1),
			ITM(123.46, 123.50, 5, 1),
			ITM(1.234'567'89, 1.234'567'900, 8, 2),
			ITM(123.456, 123.5, 5, 1),

			ITM(0.0, 0.0),
			ITM(1.454'4, 1.0, 1, 0),
			ITM(1.454'4, 1.0, 3, 0),
			ITM(1.454'4, 1.5, 2, 1),
			ITM(1.454'4, 1.5, 3, 1),
			ITM(1.454'4, 1.45, 3, 2),
			ITM(14.544, 10.0, 1, 0),
			ITM(14.544, 15.0, 2, 1),
			ITM(14.544, 15.0, 3, 0),
			ITM(14.544, 14.5, 3, 1),
			ITM(14.544, 14.5, 3, 2),
			ITM(145.44, 100.0, 1, 0),
			ITM(145.44, 145.0, 3, 0),
			ITM(145.44, 145.0, 3, 1),
			ITM(145.44, 145.0, 3, 2),
			ITM(0.145'4, 0.10, 1, 0),
			ITM(0.145'4, 0.145, 3, 0),
			ITM(0.145'4, 0.145, 3, 1),
			ITM(0.145'4, 0.145, 3, 2),
			ITM(14.544, 10.0, 1, 0),
			ITM(14.544, 15.0, 3, 0),
			ITM(14.544, 14.5, 3, 1),
			ITM(14.544, 14.5, 3, 2),
			ITM(1'454.4, 1000.0, 1, 0),
			ITM(1'454.4, 1000.0, 3, 0),
			ITM(1'454.4, 1500.0, 3, 1),
			ITM(1'454.4, 1450.0, 3, 2),
			ITM(0.014'54, 0.0100, 1, 0),
			ITM(0.014'54, 0.0150, 3, 0),
			ITM(0.014'54, 0.0145, 3, 1),
			ITM(0.014'54, 0.0145, 3, 2),
			ITM(9.544'4, 10.0, 1, 0),
			ITM(9.544'4, 10.0, 2, 0),
			ITM(9.544'4, 10.0, 3, 0),
			ITM(9.544'4, 9.5, 3, 1),
			ITM(9.544'4, 9.54, 3, 2),
			ITM(9.454'4, 9.0, 1, 0),
			ITM(9.454'4, 9.0, 2, 0),
			ITM(9.454'4, 9.0, 3, 0),
			ITM(9.454'4, 9.5, 3, 1),
			ITM(9.454'4, 9.45, 3, 2),
#	undef ITM
		};

		for (auto& i : samples)
		{	const auto rounded = round_triple(i.original_, i.precision_, i.dec_);
			if (std::max(std::abs(rounded), std::abs(i.expected_)) * DBL_EPSILON < std::abs(rounded - i.expected_))
			{	std::stringstream buff;
				buff.imbue(std::locale(std::cout.getloc(), new space_out));
				buff << std::fixed << std::setprecision(8) <<
					"Line " << i.line_ << ": " << int(i.precision_) << '/' << int(i.dec_) << '\n' <<
					"Original:\t" << i.original_ << '\n' <<
					"Expected:\t" << i.expected_ << '\n' <<
					"Rounded:\t" << rounded << '\n' <<
					std::endl;

				std::cerr << buff.str();
				assert(false);
			}
		}

		return 0;

	}(); // const auto unit_test_round

	const auto unit_test_to_string = []
	{
		static constexpr struct
		{	int line_;
			duration_t original_;
			std::string_view expected_;
			unsigned char precision_{ 2 };
			unsigned char dec_{ 1 };
		} //-V802 //-V730
		samples[] = {
#	define ITM(v, ...) {__LINE__, (v), __VA_ARGS__}
			ITM(0s, "0.0ps"),
			ITM(0ms, "0.00ps", 3, 2),
			ITM(9.4_ps, "0.0ps"),
			ITM(9.9999_ps, "10.0ps"), // The lower limit of accuracy is 10ps.
			ITM(0.1_ps, "0.0ps"),
			ITM(1.0_ps, "0.0ps"),
			ITM(10.0_ps, "10.0ps"),
			ITM(11.0_ps, "11.0ps"),
			ITM(1454.4us, "1ms", 3, 0),
			ITM(1.4544ms, "1.5ms", 3, 1),
			ITM(1.4544ms, "1.45ms", 3, 2),

			ITM(0s, "0.0ps"),
			ITM(0.01234567891s, "12.346ms", 6, 0),
			ITM(0.01234567891s, "12.35ms", 5, 2),
			ITM(0.1_ps, "0.0ps"),
			ITM(1_ps, "0.0ps"), // The lower limit of accuracy is 10ps.
			ITM(10.01ms, "10.0ms"),
			ITM(10.1ms, "10.0ms"),
			ITM(10_ps, "10.0ps"),
			ITM(100_ps, "100.0ps"),
			ITM(100ms, "100.0ms"),
			ITM(100ns, "100.0ns"),
			ITM(100s, "100.0s "),
			ITM(100us, "100.0us"),
			ITM(10ms, "10.0ms"),
			ITM(10ns, "10.0ns"),
			ITM(10s, "10.0s "),
			ITM(10us, "10.0us"),
			ITM(12.34567891s, "12.346s ", 6, 0),
			ITM(12.34567891s, "12.35s ", 5, 2),
			ITM(123.456789_ks, "123.457ks", 6, 0),
			ITM(123.4ns, "100ns", 1, 0),
			ITM(123.4ns, "120.0ns", 2, 1),
			ITM(123.4ns, "120.0ns", 2),
			ITM(123.4ns, "123.00ns", 3, 2),
			ITM(123.4ns, "123.0ns", 3),
			ITM(123.4ns, "123.40ns", 4, 2),
			ITM(123.4ns, "123.4ns", 4),
			ITM(1234.56789_ks, "1.2Ms", 3, 1),
			ITM(1h, "3.6ks"),
			ITM(1min, "60.0s "),
			ITM(1ms, "1.0ms"),
			ITM(1ns, "1.0ns"),
			ITM(1s, "1.0s "),
			ITM(1us, "1.0us"),
			ITM(4.499999999999ns, "4.50ns", 3, 2),
			ITM(4.499999999999ns, "4.50ns", 4, 2),
			ITM(4.499999999999ns, "4.5ns", 2, 1),
			ITM(4.499999999999ns, "4.5ns", 2),
			ITM(4.499999999999ns, "4.5ns", 3),
			ITM(4.499999999999ns, "4.5ns", 4),
			ITM(4.499999999999ns, "4ns", 1, 0),
			ITM(4.999999999999_ps, "0.00ps", 3, 2),
			ITM(4.999999999999_ps, "0.00ps", 4, 2),
			ITM(4.999999999999_ps, "0.0ps", 2, 1),
			ITM(4.999999999999_ps, "0.0ps", 2),
			ITM(4.999999999999_ps, "0.0ps", 3),
			ITM(4.999999999999_ps, "0.0ps", 4),
			ITM(4.999999999999_ps, "0ps", 1, 0),
			ITM(4.999999999999ns, "5.00ns", 3, 2),
			ITM(4.999999999999ns, "5.00ns", 4, 2),
			ITM(4.999999999999ns, "5.0ns", 2, 1),
			ITM(4.999999999999ns, "5.0ns", 2),
			ITM(4.999999999999ns, "5.0ns", 3),
			ITM(4.999999999999ns, "5.0ns", 4),
			ITM(4.999999999999ns, "5ns", 1, 0),
			ITM(5.000000000000_ps, "0.00ps", 3, 2),
			ITM(5.000000000000_ps, "0.00ps", 4, 2),
			ITM(5.000000000000_ps, "0.0ps", 2),
			ITM(5.000000000000_ps, "0.0ps", 3),
			ITM(5.000000000000_ps, "0.0ps", 4),
			ITM(5.000000000000_ps, "0ps", 1, 0),
			ITM(5.000000000000ns, "5.00ns", 3, 2),
			ITM(5.000000000000ns, "5.00ns", 4, 2),
			ITM(5.000000000000ns, "5.0ns", 2, 1),
			ITM(5.000000000000ns, "5.0ns", 2),
			ITM(5.000000000000ns, "5.0ns", 3),
			ITM(5.000000000000ns, "5.0ns", 4),
			ITM(5.000000000000ns, "5ns", 1, 0),
			//**********************************
			ITM(0.0_ps, "0.0ps"),
			ITM(0.123456789us, "123.5ns", 4),
			ITM(1.23456789s, "1s ", 1, 0),
			ITM(1.23456789s, "1.2s ", 3),
			ITM(1.23456789s, "1.2s "),
			ITM(1.23456789us, "1.2us"),
			ITM(1004.4ns, "1.0us", 2),
			ITM(12.3456789s, "10s ", 1, 0),
			ITM(12.3456789s, "12.3s ", 3),
			ITM(12.3456789us, "12.3us", 3),
			ITM(12.3456s, "12.0s "),
			ITM(12.34999999ms, "10ms", 1, 0),
			ITM(12.34999999ms, "12.3ms", 3),
			ITM(12.34999999ms, "12.3ms", 4),
			ITM(12.4999999ms, "12.0ms"),
			ITM(12.4999999ms, "12.5ms", 3),
			ITM(12.5000000ms, "13.0ms"),
			ITM(123.456789ms, "123.0ms", 3),
			ITM(123.456789us, "120.0us"),
			ITM(123.4999999ms, "123.5ms", 4),
			ITM(1234.56789us, "1.2ms"),
			ITM(245.0_ps, "250.0ps"),
			ITM(49.999_ps, "50.0ps"),
			ITM(50.0_ps, "50.0ps"),
			ITM(9.49999_ps, "0.0ps"),
			ITM(9.9999_ps, "10.0ps"), // The lower limit of accuracy is 10ps.
			ITM(9.999ns, "10.0ns"),
			ITM(99.49999_ps, "99.0ps"),
			ITM(99.4999ns, "99.0ns"),
			ITM(99.4ms, "99.0ms"),
			ITM(99.5_ps, "100.0ps"),
			ITM(99.5ms, "100.0ms"),
			ITM(99.5ns, "100.0ns"),
			ITM(99.5us, "100.0us"),
			ITM(99.999_ps, "100.0ps"),
			ITM(999.0_ps, "1.0ns"),
			ITM(999.45ns, "1us", 1, 0),
			ITM(999.45ns, "1.0us", 2),
			ITM(999.45ns, "999.0ns", 3),
			ITM(999.45ns, "999.5ns", 4),
			ITM(999.45ns, "999.45ns", 5, 2),
			ITM(999.55ns, "1.0us", 3),
			ITM(99ms, "99.0ms"),
		};

		for (auto& i : samples)
		{	const auto result = to_string(i.original_, i.precision_, i.dec_);
			if (i.expected_ != result)
			{	std::stringstream buff;
				buff.imbue(std::locale(std::cout.getloc(), new space_out));
				buff << std::fixed << std::setprecision(8) <<
				"Line " << i.line_ << ": " << int(i.precision_) << '/' << int(i.dec_) <<
				"\nOriginal:\t" << i.original_<<
				"\nExpected:\t" << i.expected_ <<
				"\nRounded:\t" << result <<
				'\n' << std::endl;
					
				std::cerr << buff.str();
				assert(false);
			}
		}

		return 0;

	}(); // const auto unit_test_to_string
#endif // #ifndef NDEBUG

	inline std::ostream& operator<<(std::ostream& os, const duration_t& d)
	{	return os << to_string(d);
	}

	void warming(int all, ch::milliseconds ms)
	{
		if (ms.count())
		{	std::atomic_bool done = false;
			auto load = [&done] {while (!done) {/**/ }}; //-V776

			const auto hwcnt = std::thread::hardware_concurrency();
			std::vector<std::thread> threads((0 != all && 1 < hwcnt) ? hwcnt - 1 : 0);
			for (auto& t : threads)
			{	t = std::thread{ load };
			}

			for (const auto stop = ch::steady_clock::now() + ms; ch::steady_clock::now() < stop;)
			{/*The thread is fully loaded.*/
			}

			done = true;

			for (auto& t : threads)
			{	t.join();
			}
		}
	}

	duration_t seconds_per_tick()
	{
		auto wait_for_the_time_changed = []
		{
			{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
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
			// The order of calling the functions is deliberately broken. To push 'vi_tmGetTicks()' and 'vi_tmFinish()' further apart.
			auto itm = vi_tmStart("", 1);
			vi_tmFinish(&itm);
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
		constexpr auto CNT = 100U;

		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		auto e = vi_tmGetTicks(); // Preloading a function into cache
		auto s = vi_tmGetTicks();
		for (auto cnt = CNT; cnt; --cnt)
		{
			e = vi_tmGetTicks(); //-V519
			e = vi_tmGetTicks(); //-V519
		}
		const auto pure = e - s;

		constexpr auto EXT = 20U;
		std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
		e = vi_tmGetTicks(); // Preloading a function into cache
		s = vi_tmGetTicks();
		for (auto cnt = CNT; cnt; --cnt)
		{
			e = vi_tmGetTicks(); //-V761
			e = vi_tmGetTicks();

			// EXT calls
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
			e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks(); e = vi_tmGetTicks();
		}
		const auto dirty = e - s;

		return static_cast<double>(dirty - pure) / (EXT * CNT);
	}
VI_OPTIMIZE_ON

	const std::string title_name_{ "Name"s };
	const std::string title_average_{ "Average"s };
	const std::string title_total_{ "Total"s };
	const std::string title_amount_{ "Amount"s };
	const std::string Ascending { " (^)"s };
	const std::string Descending{ " (v)"s };

	struct traits_t
	{
		struct itm_t
		{
			std::string_view on_name_; // Name
			vi_tmTicks_t on_total_{}; // Total ticks duration
			std::size_t on_amount_{}; // Number of measured units
			std::size_t on_calls_cnt_{}; // To account for overheads

			duration_t total_time_{.0}; // seconds
			duration_t average_{.0}; // seconds
			std::string total_txt_{ "<too few>" };
			std::string average_txt_{ "<too few>" };
			itm_t(const char* n, vi_tmTicks_t t, std::size_t a, std::size_t c) noexcept : on_name_{ n }, on_total_{ t }, on_amount_{ a }, on_calls_cnt_{ c } {}
		};

		std::vector<itm_t> meterages_;

		std::uint32_t flags_;
		const duration_t tick_duration_ = seconds_per_tick();
		const double measurement_cost_ = measurement_cost(); // ticks
		std::size_t max_amount_{};
		std::size_t max_len_name_{ title_name_.length()};
		std::size_t max_len_total_{ title_total_.length() };
		std::size_t max_len_average_{ title_average_.length() };
		std::size_t max_len_amount_{ title_amount_.length() };
		
		traits_t(std::uint32_t flags) : flags_{ flags }
		{
			const auto ac = Descending.length();
			assert(ac == Ascending.length());

			switch (flags_ & static_cast<uint32_t>(vi_tmSortMask))
			{
			case vi_tmSortByAmount:
				max_len_amount_ += ac;
				break;
			case vi_tmSortByName:
				max_len_name_ += ac;
				break;
			case vi_tmSortByTime:
				max_len_total_ += ac;
				break;
			case vi_tmSortBySpeed:
			default:
				max_len_average_ += ac;
				break;
			}
		}
	};

	int collector_meterages(const char* name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt, void* data)
	{
		assert(data);
		auto& traits = *static_cast<traits_t*>(data);
		assert(calls_cnt && amount >= calls_cnt);

		const auto sort = traits.flags_ & static_cast<uint32_t>(vi_tmSortMask);

		auto& itm = traits.meterages_.emplace_back(name, total, amount, calls_cnt);

		constexpr auto dirty = 1.0; // A fair odds would be 1.0
		if (const auto total_over_ticks = traits.measurement_cost_ * dirty * itm.on_calls_cnt_; itm.on_total_ > total_over_ticks)
		{
			itm.total_time_ = traits.tick_duration_ * (itm.on_total_ - total_over_ticks);
			itm.average_ = itm.total_time_ / itm.on_amount_;
			itm.total_txt_ = to_string(itm.total_time_);
			itm.average_txt_ = to_string(itm.average_);
		}

		traits.max_len_total_ = std::max(traits.max_len_total_, itm.total_txt_.length());
		traits.max_len_average_ = std::max(traits.max_len_average_, itm.average_txt_.length());
		traits.max_len_name_ = std::max(traits.max_len_name_, itm.on_name_.length());

		if (itm.on_amount_ > traits.max_amount_)
		{
			traits.max_amount_ = itm.on_amount_;
			auto max_len_amount = static_cast<std::size_t>(std::floor(std::log10(itm.on_amount_)));
			max_len_amount += max_len_amount / 3; // for thousand separators
			max_len_amount += 1U;
			traits.max_len_amount_ = max_len_amount;
		}

		return 1; // Continue enumerate.
	}

	template<vi_tmReportFlags E> auto make_tuple(const traits_t::itm_t& v);
	template<vi_tmReportFlags E> bool less(const traits_t::itm_t& l, const traits_t::itm_t& r)
	{	return make_tuple<E>(r) < make_tuple<E>(l);
	}

	template<> auto make_tuple<vi_tmSortByName>(const traits_t::itm_t& v)
	{	return std::tuple{ v.on_name_ };
	}
	template<> auto make_tuple<vi_tmSortBySpeed>(const traits_t::itm_t& v)
	{	return std::tuple{ v.average_, v.total_time_, v.on_amount_, v.on_name_ };
	}
	template<> auto make_tuple<vi_tmSortByTime>(const traits_t::itm_t& v)
	{	return std::tuple{ v.total_time_, v.average_, v.on_amount_, v.on_name_ };
	}
	template<> auto make_tuple<vi_tmSortByAmount>(const traits_t::itm_t& v)
	{	return std::tuple{ v.on_amount_, v.average_, v.total_time_, v.on_name_ };
	}

	struct meterage_comparator_t
	{
		uint32_t flags_{};

		explicit meterage_comparator_t(uint32_t flags) noexcept : flags_{ flags } {}

		bool operator ()(const traits_t::itm_t& l, const traits_t::itm_t& r) const {
			auto pr = &less<vi_tmSortBySpeed>;
			switch (flags_ & static_cast<uint32_t>(vi_tmSortMask))
			{
			case static_cast<uint32_t>(vi_tmSortByName):
				pr = less<vi_tmSortByName>;
				break;
			case static_cast<uint32_t>(vi_tmSortByAmount):
				pr = less<vi_tmSortByAmount>;
				break;
			case static_cast<uint32_t>(vi_tmSortByTime):
				pr = less<vi_tmSortByTime>;
				break;
			case static_cast<uint32_t>(vi_tmSortBySpeed):
				break;
			default:
				assert(false);
				break;
			}

			const bool desc = (0 == (static_cast<uint32_t>(vi_tmSortAscending) & flags_));
			return desc ? pr(l, r) : pr(r, l);
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
			const auto order = (traits_.flags_ & static_cast<uint32_t>(vi_tmSortAscending)) ? Ascending : Descending;
			auto sort = vi_tmSortBySpeed;
			switch (auto s = traits_.flags_ & static_cast<uint32_t>(vi_tmSortMask))
			{
			case vi_tmSortBySpeed:
				break;
			case vi_tmSortByAmount:
			case vi_tmSortByName:
			case vi_tmSortByTime:
				sort = static_cast<vi_tmReportFlags>(s);
				break;
			default:
				assert(false);
				break;
			}

			std::ostringstream str;
			str << std::setw(number_len_) << "#" << "  ";
			str << std::setw(traits_.max_len_name_) << std::left << (title_name_ + (sort == vi_tmSortByName ? order : "")) << ": ";
			str << std::setw(traits_.max_len_average_) << std::right << (title_average_ + (sort == vi_tmSortBySpeed ? order : "")) << std::setfill(' ') << " [";
			str << std::setw(traits_.max_len_total_) << (title_total_ + (sort == vi_tmSortByTime ? order : "")) << " / ";
			str << std::setw(traits_.max_len_amount_) << (title_amount_ + (sort == vi_tmSortByAmount ? order : "")) << "]";
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

			const char fill = (traits_.meterages_.size() > 4 && 0 != (n_ - 1) % 2) ? ' ' : '.';
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
	warming(false, 512ms);

	traits_t traits{ flags };
	vi_tmResults(collector_meterages, &traits);

	std::sort(traits.meterages_.begin(), traits.meterages_.end(), meterage_comparator_t{ flags });

	int ret = 0;
	if (flags & static_cast<std::uint32_t>(vi_tmShowMask))
	{	std::ostringstream str;

		if (flags & static_cast<uint32_t>(vi_tmShowOverhead))
		{	str << "Measurement cost: " << traits.tick_duration_ * traits.measurement_cost_ << " per measurement. ";
		}
		if (flags & static_cast<uint32_t>(vi_tmShowDuration))
		{	str << "Duration: " << duration() << ". ";
		}
		if (flags & static_cast<uint32_t>(vi_tmShowUnit))
		{	str << "One tick corresponds: " << traits.tick_duration_ << ". ";
		}

		str << '\n';
		ret = fn(str.str().c_str(), data);
	}

	meterage_format_t mf{ traits, fn, data };
	ret += mf.header();
	return std::accumulate(traits.meterages_.begin(), traits.meterages_.end(), ret, mf);
}

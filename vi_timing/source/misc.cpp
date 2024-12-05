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

#include "internal.h"

#include <vi_timing.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <vector>
#include <thread>
#include <string_view>

namespace ch = std::chrono;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

namespace
{
	int mod_normalize(int num, int den)
	{	assert(den > 0); // num may be negative!
		auto result = num % den; // Now: -den < result && result < den;
		if (result <= 0)
		{	result += den;
		}
		return result; // Now: 0 < result && result <= den
	}

	int prec_restrict(int prec, int exp, int dec, int group)
	{	if (const auto num_ext = mod_normalize(exp, group); num_ext < prec)
		{	if (const auto prec_ext = (prec - num_ext) % group; prec_ext > dec)
			{	prec -= prec_ext - dec;
			}
		}
		return prec;
	}

	constexpr auto operator""_ps(long double v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ps(unsigned long long v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ks(long double v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_ks(unsigned long long v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_Ms(long double v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Ms(unsigned long long v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Gs(long double v) noexcept { return ch::duration<double, std::giga>(v); };
	constexpr auto operator""_Gs(unsigned long long v) noexcept { return ch::duration<double, std::giga>(v); };
}

[[nodiscard]] double misc::round_ext(double num, int prec, int dec, int group)
{ // Rounding to 'dec' decimal place and no more than 'prec' significant symbols.
	constexpr auto EPS = std::numeric_limits<decltype(num)>::epsilon();
	assert(num >= .0 && dec >= 0 && group > 0 && prec > dec && group > dec);
	if (num > .0 && dec >= 0 && group > 0 && prec > dec)
	{	dec %= group;
		const auto exp = static_cast<int>(std::ceil(std::log10(num)));
		assert(0.1 * (1.0 - EPS) <= num * std::pow(10, -exp) && num * std::pow(10, -exp) < 1.0 + EPS);
		const auto factor = std::pow(10, prec_restrict(prec, exp, dec, group) - exp);
		num = std::round(num * (1.0 + EPS) * factor) / factor;
	}
	return num;
}

[[nodiscard]] std::string misc::to_string(misc::duration_t d, unsigned char precision, unsigned char dec)
{
	assert(d.count() >= 0.0 && dec >= 0 && precision > dec);
	auto sec = round_ext(d.count(), precision, dec);
	const auto triple = static_cast<int>(std::floor(std::log10(sec))) - 3 * ((precision - dec - 1) / 3);

	struct { std::string_view suffix_; double factor_; } k{ "ps"sv, 1e12 };
	if (-11 > triple) { sec = 0.0; }
	else if (-9 > triple) { k = { "ps"sv, 1e12 }; }
	else if (-6 > triple) { k = { "ns"sv, 1e9 }; }
	else if (-3 > triple) { k = { "us"sv, 1e6 }; }
	else if (0 > triple) { k = { "ms"sv, 1e3 }; }
	else if (+3 > triple) { k = { "s "sv, 1e0 }; }
	else if (+6 > triple) { k = { "ks"sv, 1e-3 }; }
	else if (+9 > triple) { k = { "Ms"sv, 1e-6 }; }
	else { k = { "Gs"sv, 1e-9 }; }

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(dec) << sec * k.factor_ << ' ' << k.suffix_;
	return ss.str();
}

void VI_TM_CALL vi_tmWarming(unsigned int threads_qty, unsigned int ms)
{
	if (0 == ms)
	{	return;
	}

	static const auto cores_qty = std::thread::hardware_concurrency();
	auto addition = threads_qty? std::min(threads_qty, cores_qty): cores_qty;
	if (addition)
	{	--addition; // subtract the current thread
	}

	std::atomic_bool done = false;
	auto thread_func = [&done]
		{	while (!done) //-V776 "Potentially infinite loop."
			{	for (volatile int n = 100'000; n; n = n - 1)
				{/**/}
			}
		};

	std::vector<std::thread> threads(addition); // Additional threads
	for (auto &t : threads)
	{	t = std::thread{ thread_func };
	}

	for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ ms }; ch::steady_clock::now() < stop;)
	{	// Stressing the current thread.
	}

	done = true;

	for (auto &t : threads)
	{	t.join();
	}
}

namespace
{
#ifndef NDEBUG
	template<typename T, typename std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
	constexpr bool equal(T l, T r)
	{	if (std::isnan(l) || std::isnan(r))
		{	return false;
		}
		return std::abs(l - r) <= std::max(std::abs(l), std::abs(r)) * std::numeric_limits<T>::epsilon();
	}

	const auto unit_test_round = []
		{
			static const struct
			{
				int line_; //-V802 "On 32-bit/64-bit platform, structure size can be reduced..."
				double original_;
				double expected_;
				unsigned char precision_;
				unsigned char dec_;
			}
			samples[] = {
				{ __LINE__, 9.999'9, 10.0, 1, 0 },
				{ __LINE__, 9.999'9, 10.0, 2, 0 },
				{ __LINE__, 9.999'9, 10.0, 3, 0 },
				{ __LINE__, 9.999'9, 10.0, 4, 0 },
				{ __LINE__, 9.999'9, 10.0, 2, 1 },
				{ __LINE__, 9.999'9, 10.0, 3, 1 },
				{ __LINE__, 9.999'9, 10.0, 4, 1 },
				{ __LINE__, 9.999'9, 10.0, 3, 2 },
				{ __LINE__, 9.999'9, 10.0, 4, 2 },
#	define ITM(E, precition, dec, expected) {__LINE__, 5.555'555'555'555 * std::pow(10, E), expected, precition, dec}
				ITM(-3, 1, 0, 0.006),
				ITM(-2, 1, 0, 0.060),
				ITM(-1, 1, 0, 0.600),
				ITM(0, 1, 0, 6.000),
				ITM(1, 1, 0, 60.000),
				ITM(2, 1, 0, 600.000),

				ITM(-3, 2, 0, 0.006),
				ITM(-2, 2, 0, 0.056),
				ITM(-1, 2, 0, 0.560),
				ITM(0, 2, 0, 6.000),
				ITM(1, 2, 0, 56.000),
				ITM(2, 2, 0, 560.000),

				ITM(-3, 3, 0, 0.006),
				ITM(-2, 3, 0, 0.056),
				ITM(-1, 3, 0, 0.556),
				ITM(0, 3, 0, 6.000),
				ITM(1, 3, 0, 56.000),
				ITM(2, 3, 0, 556.000),

				ITM(-3, 4, 0, 0.005'556),
				ITM(-2, 4, 0, 0.056'000),
				ITM(-1, 4, 0, 0.556'000),
				ITM(0, 4, 0, 5.556'000),
				ITM(1, 4, 0, 56.000'000),
				ITM(2, 4, 0, 556.000'000),

				ITM(-3, 5, 0, 0.005'556),
				ITM(-2, 5, 0, 0.055'556),
				ITM(-1, 5, 0, 0.556'000),
				ITM(0, 5, 0, 5.556'000),
				ITM(1, 5, 0, 55.556'000),
				ITM(2, 5, 0, 556.000'000),

				ITM(-3, 6, 0, 0.005'556),
				ITM(-2, 6, 0, 0.055'556),
				ITM(-1, 6, 0, 0.555'556),
				ITM(0, 6, 0, 5.556'000),
				ITM(1, 6, 0, 55.556'000),
				ITM(2, 6, 0, 555.556'000),

				ITM(-3, 2, 1, 0.005'6),
				ITM(-2, 2, 1, 0.056'0),
				ITM(-1, 2, 1, 0.560'0),
				ITM(0, 2, 1, 5.600'0),
				ITM(1, 2, 1, 56.000'0),
				ITM(2, 2, 1, 560.000'0),

				ITM(-3, 3, 1, 0.005'6),
				ITM(-2, 3, 1, 0.055'6),
				ITM(-1, 3, 1, 0.556'0),
				ITM(0, 3, 1, 5.600'0),
				ITM(1, 3, 1, 55.600'0),
				ITM(2, 3, 1, 556.000'0),

				ITM(-3, 4, 1, 0.005'556),
				ITM(-2, 4, 1, 0.055'600),
				ITM(-1, 4, 1, 0.555'600),
				ITM(0, 4, 1, 5.556'000),
				ITM(1, 4, 1, 55.600'000),
				ITM(2, 4, 1, 555.600'000),

				ITM(-3, 5, 1, 0.005'555'6),
				ITM(-2, 5, 1, 0.055'556'0),
				ITM(-1, 5, 1, 0.555'600'0),
				ITM(0, 5, 1, 5.555'600'0),
				ITM(1, 5, 1, 55.556'000'0),
				ITM(2, 5, 1, 555.600'000'0),

				ITM(-3, 6, 1, 0.005'555'6),
				ITM(-2, 6, 1, 0.055'555'6),
				ITM(-1, 6, 1, 0.555'556'0),
				ITM(0, 6, 1, 5.555'600'0),
				ITM(1, 6, 1, 55.555'600'0),
				ITM(2, 6, 1, 555.556'000'0),

				ITM(-3, 3, 2, 0.005'56),
				ITM(-2, 3, 2, 0.055'60),
				ITM(-1, 3, 2, 0.556'00),
				ITM(0, 3, 2, 5.560'00),
				ITM(1, 3, 2, 55.600'00),
				ITM(2, 3, 2, 556.000'00),

				ITM(-3, 4, 2, 0.005'556),
				ITM(-2, 4, 2, 0.055'560),
				ITM(-1, 4, 2, 0.555'600),
				ITM(0, 4, 2, 5.556'000),
				ITM(1, 4, 2, 55.560'000),
				ITM(2, 4, 2, 555.600'000),

				ITM(-3, 5, 2, 0.005'555'6),
				ITM(-2, 5, 2, 0.055'556'0),
				ITM(-1, 5, 2, 0.555'560'0),
				ITM(0, 5, 2, 5.555'600'0),
				ITM(1, 5, 2, 55.556'000'0),
				ITM(2, 5, 2, 555.560'000'0),

				ITM(-3, 6, 2, 0.005'555'56),
				ITM(-2, 6, 2, 0.055'555'60),
				ITM(-1, 6, 2, 0.555'556'00),
				ITM(0, 6, 2, 5.555'560'00),
				ITM(1, 6, 2, 55.555'600'00),
				ITM(2, 6, 2, 555.556'000'00),
#	undef ITM
			};

			for (auto &i : samples)
			{
				const auto rounded = misc::round_ext(i.original_, i.precision_, i.dec_);
				if (!equal(rounded, i.expected_))
				{
					std::ostringstream buff;
					buff.imbue(std::locale(std::cout.getloc(), new misc::space_out));
					buff <<
						std::fixed << std::setprecision(8) <<
						"Line " << i.line_ << ": " << int(i.precision_) << '/' << int(i.dec_) << "\n"
						"Original:\t" << i.original_ << "\n"
						"Expected:\t" << i.expected_ << "\n"
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
			static const struct
			{
				int line_{}; //-V802 "On 32-bit/64-bit platform, structure size can be reduced..."
				misc::duration_t original_;
				std::string_view expected_;
				unsigned char precision_ = 2;
				unsigned char dec_ = 1;
			}
			samples[] = {
#	define ITM5(E, precition, dec, expected) {__LINE__, 5.555'555'555'555 * std::pow(10, E), expected, precition, dec}
				ITM5(-2, 5, 0, "55556 us"),
				ITM5(-9, 4, 1, "5.6 ns"),
				ITM5(0, 2, 1, "5.6 s "),
				ITM5(-12, 2, 1, "0.0 ps"),
				ITM5(-9, 3, 1, "5.6 ns"),
				ITM5(-2, 5, 2, "55.56 ms"),
				ITM5(-4, 5, 2, "555.56 us"), //-V112 "Dangerous magic number N used."
#	undef ITM5
#	define ITM(v, ...) {__LINE__, (v), __VA_ARGS__}
				{ __LINE__, 0.0_ps, "0.0 ps" },
				{ __LINE__, 0.123456789us, "123.5 ns", 4 },
				{ __LINE__, 1.23456789s, "1 s ", 1, 0 },
				{ __LINE__, 1.23456789s, "1.2 s ", 3 },
				{ __LINE__, 1.23456789s, "1.2 s " },
				{ __LINE__, 1.23456789us, "1.2 us" },
				{ __LINE__, 1004.4ns, "1.0 us", 2 },
				{ __LINE__, 12.3456789s, "10 s ", 1, 0 },
				{ __LINE__, 12.3456789s, "12.3 s ", 3 },
				{ __LINE__, 12.3456789us, "12.3 us", 3 },
				{ __LINE__, 12.3456s, "12.0 s " },
				{ __LINE__, 12.34999999ms, "10 ms", 1, 0 },
				{ __LINE__, 12.34999999ms, "12.3 ms", 3 },
				{ __LINE__, 12.34999999ms, "12.3 ms", 4 },
				{ __LINE__, 12.4999999ms, "12.0 ms" },
				{ __LINE__, 12.4999999ms, "12.5 ms", 3 },
				{ __LINE__, 12.5000000ms, "13.0 ms" },
				{ __LINE__, 123.456789ms, "123.0 ms", 3 },
				{ __LINE__, 123.456789us, "120.0 us" },
				{ __LINE__, 123.4999999ms, "123.5 ms", 4 },
				{ __LINE__, 1234.56789us, "1.2 ms" },
				{ __LINE__, 245.0_ps, "250.0 ps" },
				{ __LINE__, 49.999_ps, "50.0 ps" },
				{ __LINE__, 50.0_ps, "50.0 ps" },
				{ __LINE__, 9.49999_ps, "0.0 ps" },
				{ __LINE__, 9.9999_ps, "10.0 ps" }, // The lower limit of accuracy is 10ps.
				{ __LINE__, 9.999ns, "10.0 ns" },
				{ __LINE__, 99.49999_ps, "99.0 ps" },
				{ __LINE__, 99.4999ns, "99.0 ns" },
				{ __LINE__, 99.4ms, "99.0 ms" },
				{ __LINE__, 99.5_ps, "100.0 ps" },
				{ __LINE__, 99.5ms, "100.0 ms" },
				{ __LINE__, 99.5ns, "100.0 ns" },
				{ __LINE__, 99.5us, "100.0 us" },
				{ __LINE__, 99.999_ps, "100.0 ps" },
				{ __LINE__, 999.0_ps, "1.0 ns" },
				{ __LINE__, 999.45ns, "1 us", 1, 0 },
				{ __LINE__, 999.45ns, "1.0 us", 2 },
				{ __LINE__, 999.45ns, "999.0 ns", 3 },
				{ __LINE__, 999.45ns, "999.5 ns", 4 },
				{ __LINE__, 999.45ns, "999.45 ns", 5, 2 },
				{ __LINE__, 999.55ns, "1.0 us", 3 },
				{ __LINE__, 99ms, "99.0 ms" },
#	undef ITM
			};

			for (auto &i : samples)
			{
				const auto result = misc::to_string(i.original_, i.precision_, i.dec_);
				if (i.expected_ != result)
				{
					std::ostringstream buff;
					buff.imbue(std::locale(std::cout.getloc(), new misc::space_out));
					buff <<
						std::setprecision(8) <<
						"Line " << i.line_ << ": " << int(i.precision_) << '/' << int(i.dec_) << "\n"
						"Original:\t" << i.original_.count() << "\n" <<
						std::fixed <<
						"Expected:\t" << i.expected_ << "\n"
						"Rounded:\t" << result << '\n' <<
						std::endl;

					std::cerr << buff.str();
					assert(false);
				}
			}

			return 0;

		}(); // const auto unit_test_to_string
#endif // #ifndef NDEBUG
}

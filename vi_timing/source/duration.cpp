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

#include "duration.h"
#include "build_number_maker.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace
{
	/// <summary>
	/// rounding a floating-point number to a specified number of significant digits
	/// </summary>
	/// <param name="num">The number to be rounded.</param>
	/// <param name="prec">The number of significant digits to round to.</param>
	/// <returns>The rounded number.</returns>
	/// <remarks>
	/// This function ensures that the number is not NaN, infinity, or zero before rounding.
	/// </remarks>
	[[nodiscard]] double round_ext(double num, unsigned char prec)
	{	assert(prec > 0);
		if (0 == prec || std::isnan(num) || std::isinf(num) || 0.0 == num)
			return num;

		const auto exp = std::ceil(std::log10(std::abs(num)));
		const auto factor = std::pow(10.0, prec - exp);
		return std::round(num * factor) / factor;
	}
}

/// <summary>
/// Converts a duration to a string representation with specified precision and decimal places.
/// </summary>
/// <param name="sec">The duration to be converted.</param>
/// <param name="prec">The number of significant digits to round to.</param>
/// <param name="dec">The number of decimal places to display.</param>
/// <returns>A string representation of the duration.</returns>
/// <remarks>
/// This function handles special cases such as NaN, infinity values.
/// It rounds the duration to the specified precision and formats it with the appropriate unit suffix.
/// </remarks>
[[nodiscard]] std::string misc::to_string(misc::duration_t sec, unsigned char significant, unsigned char decimal)
{	assert(decimal < significant);

	std::string result;
	
	if (auto num = sec.count(); std::isnan(num))
	{	result = "NaN";
	}
	else if (std::isinf(num))
	{	result = (num > .0) ? "INF" : "-INF";
	}
	else
	{	num = round_ext(num, significant);

		struct
		{	std::string_view suffix_;
			double factor_;
		} unit = { " ps", 1e12 };

		if (std::abs(num) < 1e-12)
		{	num = 0.0;
		}
		else
		{	constexpr auto GROUP = 3;
			const auto supplement = ((significant - decimal - 1) / GROUP) * GROUP - log10(std::abs(num));
			if (supplement <= -6) { unit = { " Ms", 1e-6 }; }
			else if (supplement <= -3) { unit = { " ks", 1e-3 }; }
			else if (supplement <= 0) { unit = { " s ", 1e+0 }; }
			else if (supplement <= 3) { unit = { " ms", 1e+3 }; }
			else if (supplement <= 6) { unit = { " us", 1e+6 }; }
			else if (supplement <= 9) { unit = { " ns", 1e+9 }; }
		}

		std::ostringstream ss;
		ss << std::fixed << std::setprecision(decimal) << (num * unit.factor_) << unit.suffix_;
		result = ss.str();
	}

	return result;
}

#ifndef NDEBUG
namespace
{
	const auto nanotests =
	(	[]{	// nanotest for round_ext
			assert(0.0 == round_ext(0.00, 1));

			assert(1.0 == round_ext(0.95, 1));
			assert(1.0 == round_ext(1.00, 1));
			assert(1.0 == round_ext(1.40, 1));
			assert(-1.0 == round_ext(-0.95, 1));
			assert(-1.0 == round_ext(-1.00, 1));
			assert(-1.0 == round_ext(-1.40, 1));

			assert(0.10 == round_ext(.0995, 2));
			assert(0.10 == round_ext(.1000, 2));
			assert(0.10 == round_ext(.1044, 2));
			assert(-0.10 == round_ext(-.0995, 2));
			assert(-0.10 == round_ext(-.1000, 2));
			assert(-0.10 == round_ext(-.1044, 2));

			assert(1.0e-16 == round_ext(0.95e-16, 1));
			assert(1.0e-16 == round_ext(1.00e-16, 1));
			assert(1.0e-16 == round_ext(1.40e-16, 1));
		}(),
		[]{	// nanotest for misc::to_string(duration_t d, unsigned char precision, unsigned char dec)
			const struct
			{	misc::duration_t v_;
				std::string_view expected_;
				unsigned char prec_;
				unsigned char dec_;
				int line_;
			} vals[] =
			{	
				//****************
				{ -1234.5, "-1.2 ks", 2, 1, __LINE__ },
				{ -12345.6, "-12.0 ks", 2, 1, __LINE__ },
				{ -0.0, "0 ps", 1, 0, __LINE__ },
				{ -11.0, "-10 s ", 1, 0, __LINE__ },
				{ 11.0, "10 s ", 1, 0, __LINE__ },
				{ 11.0, "11 s ", 2, 0, __LINE__ },
				{ 10.0, "10 s ", 1, 0, __LINE__ },
				{ 5.0, "5 s ", 1, 0, __LINE__ },
				{ 1.1, "1.1 s ", 2, 1, __LINE__ },
				{ 1.1, "1 s ", 1, 0, __LINE__ },
				{ 0.0, "0.0 ps", 3, 1, __LINE__ },
				{ 0.9994e-12, "0.0 ps", 3, 1, __LINE__ },
				{ 0.9996e-12, "1.0 ps", 3, 1, __LINE__ },
				{ 1e-12, "1.0 ps", 3, 1, __LINE__ },
				{ 1.2345e12, "1230000.00 Ms", 3, 2, __LINE__ },
				{ 0.555, "560.0 ms", 2, 1, __LINE__ },
				{ 5.55, "5.6 s ", 2, 1, __LINE__ },
				{ 55.5, "56.0 s ", 2, 1, __LINE__ },
				{ 123.456789,  "123457.0 ms", 6, 1, __LINE__ },
				{ 12.3456789,   "12345.7 ms", 6, 1, __LINE__ },
				{ 1.23456789,    "1234.6 ms", 6, 1, __LINE__ },
				{ 123.456789, "123456.80 ms", 7, 2, __LINE__ },
				{ 1.23456789,   "1234.57 ms", 7, 2, __LINE__ },
				{ 12.3456789,  "12345.68 ms", 7, 2, __LINE__ },
				{ 123.456789, "123456.80 ms", 7, 2, __LINE__ },
				{ 1234.56789,   "1234.57 s ", 7, 2, __LINE__ },
				{ 12345.6789,  "12345.68 s ", 7, 2, __LINE__ },
				{ 123456.789, "123456.80 s ", 7, 2, __LINE__ },
				{ 1234567.89,   "1234.57 ks", 7, 2, __LINE__ },
				{ 12345678.9,  "12345.68 ks", 7, 2, __LINE__ },
				{ 123456789., "123456.80 ks", 7, 2, __LINE__ },
				{ 1.23456, "1.2 s ", 3, 1, __LINE__ },
				{ 12.3456, "12.3 s ", 3, 1, __LINE__ },
				{ 123.456, "123.0 s ", 3, 1, __LINE__ },
				{ 1234.56, "1.2 ks", 3, 1, __LINE__ },
				{ 12.3456789e-6, "12345680 ps", 7, 0, __LINE__},
				{ 12.3456789e-6, "12345.7 ns", 7, 1, __LINE__},
				{ 12.3456789e-6, "12345.68 ns", 7, 2, __LINE__},
				{ 12.3456789e-6, "12345.680 ns", 7, 3, __LINE__},
				{ 12.3456789e-6, "12.3457 us", 7, 4, __LINE__},
				{ 12.3456789e-6, "12.34568 us", 7, 5, __LINE__},
			};

			for (auto &v : vals)
			{	const auto s = misc::to_string(v.v_, v.prec_, v.dec_);
				assert(s == v.expected_);
			}
		}(),
		0
	);
}
#endif // #ifndef NDEBUG

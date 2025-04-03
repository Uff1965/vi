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
	[[nodiscard]] double round_ext(double num, unsigned char significant)
	{	assert(significant > 0);
		if (0 == significant || std::isnan(num) || std::isinf(num) || 0.0 == num)
			return num;

		const auto exp = std::ceil(std::log10(std::abs(num)));
		const auto factor = std::pow(10.0, significant - exp);
		return std::round(num * factor) / factor;
	}
}

/// <summary>
/// Converts a duration to a string representation with specified precision and decimal places.
/// </summary>
/// <param name="sec">The duration to be converted.</param>
/// <param name="significant">The number of significant digits to round to.</param>
/// <param name="decimal">The number of decimal places to display.</param>
/// <returns>A string representation of the duration.</returns>
/// <remarks>
/// This function handles special cases such as NaN, infinity values.
/// It rounds the duration to the specified precision and formats it with the appropriate unit suffix.
/// </remarks>
[[nodiscard]] std::string misc::to_string(const misc::duration &sec, unsigned char significant, unsigned char decimal)
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
			const auto supp = ((significant - decimal - 1) / GROUP) * GROUP - log10(std::abs(num));
			if (supp <= -6) { unit = { " Ms", 1e-6 }; }
			else if (supp <= -3) { unit = { " ks", 1e-3 }; }
			else if (supp <= 0) { unit = { " s ", 1e+0 }; }
			else if (supp <= 3) { unit = { " ms", 1e+3 }; }
			else if (supp <= 6) { unit = { " us", 1e+6 }; }
			else if (supp <= 9) { unit = { " ns", 1e+9 }; }
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
	const auto nanotests = []
	{
		{	// nanotest for round_ext
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
		}

		{	// nanotest for misc::to_string(misc::duration d, unsigned char precision, unsigned char dec)
			const struct
			{
				int line_;
				double num_;
				std::string_view expected_;
				unsigned char significant_;
				unsigned char decimal_;
			} tests_set[] =
			{
				{ __LINE__, .0, "0 ps", 1, 0 },
				{ __LINE__, .0, "0.00 ps", 7, 2 },
				{ __LINE__, -9e-13, "0.00 ps", 7, 2 },
				{ __LINE__, 9e-13, "0 ps", 1, 0 },
				{ __LINE__, -1e-12, "-1 ps", 1, 0 },
				{ __LINE__, 1e-12, "1 ps", 1, 0 },
				{ __LINE__, 1e-12, "1.00 ps", 7, 2 },
				{ __LINE__, -1e9, "-1000 Ms", 1, 0 },
				{ __LINE__, 1e9, "1000 Ms", 1, 0 },
				{ __LINE__, 1e9, "1000.00 Ms", 7, 2 },

				{ __LINE__, .00555555555,        "6 ms", 1, 0 },
				{ __LINE__, .05555555555,       "60 ms", 1, 0 },
				{ __LINE__, .55555555555,      "600 ms", 1, 0 },
				{ __LINE__, 5.5555555555,        "6 s ", 1, 0 },
				{ __LINE__, 55.555555555,       "60 s ", 1, 0 },
				{ __LINE__, 555.55555555,      "600 s ", 1, 0 },
				{ __LINE__, 5555.5555555,        "6 ks", 1, 0 },

				{ __LINE__, .00555555555,        "6 ms", 2, 0 },
				{ __LINE__, .05555555555,       "56 ms", 2, 0 },
				{ __LINE__, .55555555555,      "560 ms", 2, 0 },
				{ __LINE__, 5.5555555555,        "6 s ", 2, 0 },
				{ __LINE__, 55.555555555,       "56 s ", 2, 0 },
				{ __LINE__, 555.55555555,      "560 s ", 2, 0 },
				{ __LINE__, 5555.5555555,        "6 ks", 2, 0 },

				{ __LINE__, .00555555555,        "6 ms", 3, 0 },
				{ __LINE__, .05555555555,       "56 ms", 3, 0 },
				{ __LINE__, .55555555555,      "556 ms", 3, 0 },
				{ __LINE__, 5.5555555555,        "6 s ", 3, 0 },
				{ __LINE__, 55.555555555,       "56 s ", 3, 0 },
				{ __LINE__, 555.55555555,      "556 s ", 3, 0 },
				{ __LINE__, 5555.5555555,        "6 ks", 3, 0 },

				{ __LINE__, .00555555555,     "5556 us", 4, 0 },
				{ __LINE__, .05555555555,    "55560 us", 4, 0 },
				{ __LINE__, .55555555555,   "555600 us", 4, 0 },
				{ __LINE__, 5.5555555555,     "5556 ms", 4, 0 },
				{ __LINE__, 55.555555555,    "55560 ms", 4, 0 },
				{ __LINE__, 555.55555555,   "555600 ms", 4, 0 },
				{ __LINE__, 5555.5555555,     "5556 s ", 4, 0 },

				{ __LINE__, .00555555555,      "5.6 ms", 2, 1 },
				{ __LINE__, .05555555555,     "56.0 ms", 2, 1 },
				{ __LINE__, .55555555555,    "560.0 ms", 2, 1 },
				{ __LINE__, 5.5555555555,      "5.6 s ", 2, 1 },
				{ __LINE__, 55.555555555,     "56.0 s ", 2, 1 },
				{ __LINE__, 555.55555555,    "560.0 s ", 2, 1 },
				{ __LINE__, 5555.5555555,      "5.6 ks", 2, 1 },

				{ __LINE__, .00555555555,      "5.6 ms", 3, 1 },
				{ __LINE__, .05555555555,     "55.6 ms", 3, 1 },
				{ __LINE__, .55555555555,    "556.0 ms", 3, 1 },
				{ __LINE__, 5.5555555555,      "5.6 s ", 3, 1 },
				{ __LINE__, 55.555555555,     "55.6 s ", 3, 1 },
				{ __LINE__, 555.55555555,    "556.0 s ", 3, 1 },
				{ __LINE__, 5555.5555555,      "5.6 ks", 3, 1 },

				{ __LINE__, .00555555555,      "5.6 ms", 4, 1 },
				{ __LINE__, .05555555555,     "55.6 ms", 4, 1 },
				{ __LINE__, .55555555555,    "555.6 ms", 4, 1 },
				{ __LINE__, 5.5555555555,      "5.6 s ", 4, 1 },
				{ __LINE__, 55.555555555,     "55.6 s ", 4, 1 },
				{ __LINE__, 555.55555555,    "555.6 s ", 4, 1 },
				{ __LINE__, 5555.5555555,      "5.6 ks", 4, 1 },

				{ __LINE__, .00555555555,   "5555.6 us", 5, 1 },
				{ __LINE__, .05555555555,  "55556.0 us", 5, 1 },
				{ __LINE__, .55555555555, "555560.0 us", 5, 1 },
				{ __LINE__, 5.5555555555,   "5555.6 ms", 5, 1 },
				{ __LINE__, 55.555555555,  "55556.0 ms", 5, 1 },
				{ __LINE__, 555.55555555, "555560.0 ms", 5, 1 },
				{ __LINE__, 5555.5555555,   "5555.6 s ", 5, 1 },

				{ __LINE__, .00555555555,   "5555.56 us", 6, 2 },
				{ __LINE__, .05555555555,  "55555.60 us", 6, 2 },
				{ __LINE__, .55555555555, "555556.00 us", 6, 2 },
				{ __LINE__, 5.5555555555,   "5555.56 ms", 6, 2 },
				{ __LINE__, 55.555555555,  "55555.60 ms", 6, 2 },
				{ __LINE__, 555.55555555, "555556.00 ms", 6, 2 },
				{ __LINE__, 5555.5555555,   "5555.56 s ", 6, 2 },
			};

			for (auto &test : tests_set)
			{	const auto reality = misc::to_string(misc::duration{ test.num_ }, test.significant_, test.decimal_);
				assert(reality == test.expected_);
			}
		}

		return 0;
	}();
}
#endif // #ifndef NDEBUG

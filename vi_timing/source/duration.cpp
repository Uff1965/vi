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

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace
{
	[[nodiscard]] double round_ext(double num, unsigned char prec)
	{	assert(prec);
		if (0 != prec && 0.0 != num)
		{	const auto exp = 1 + static_cast<int>(std::floor(std::log10(std::abs(num))));
			const auto factor = std::pow(10, prec - exp);
			num = std::round(num * factor) / factor;
		}

		return num;
	}

}

[[nodiscard]] std::string misc::to_string(misc::duration_t sec, unsigned char prec, unsigned char dec)
{	auto num = sec.count();
	assert(.0 <= num && 0 < prec && 0 <= dec && dec < prec);
	num = round_ext(num, prec);

	struct
	{	std::string_view suffix_;
		double factor_;
	} unit = {"ps", 1e12};

	if(1e-12 > num)
	{	num = 0.0;
	}
	else
	{	constexpr auto GROUP = 3;
		const auto order_offset = ((prec - dec - 1) / GROUP) * GROUP;
		const auto order = static_cast<int>(std::floor(std::log10(num))) - order_offset;

		if (+6 <= order) { unit = { "Ms", 1e-6 }; }
		else if (+3 <= order) { unit = { "ks", 1e-3 }; }
		else if (0 <= order) { unit = { "s ", 1e0 }; }
		else if (-3 <= order) { unit = { "ms", 1e3 }; }
		else if (-6 <= order) { unit = { "us", 1e6 }; }
		else if (-9 <= order) { unit = { "ns", 1e9 }; }
	}

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(dec) << num * unit.factor_ << ' ' << unit.suffix_;
	return ss.str();
}

#ifndef NDEBUG
namespace
{
	const auto nanotest = []
		{	const struct
			{	misc::duration_t v_;
				std::string_view expected_;
				unsigned char prec_;
				unsigned char dec_;
				int line_;
			} vals[] =
			{	
				//****************
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

			return 0;
		}();

	const auto nanotest_2 = []
		{
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

			return 0;
		}();
}
#endif // #ifndef NDEBUG

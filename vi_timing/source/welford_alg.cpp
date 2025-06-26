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

#include "welford_alg.h"
#include "build_number_maker.h" // For build number generation.

#include <cassert>
#include <cmath> // std::sqrt
#include <cstdint> // std::uint64_t, std::size_t
#include <numeric> // std::accumulate

void welford_t::update(double v_f, double n_f) noexcept
{	assert(n_f > 0.0);

	constexpr double K2 = 6.25; // 2.5^2 Threshold for outliers.
	const auto deviation = v_f / n_f - mean_; // Difference from the mean value.
	const auto sum_square_dev = deviation * deviation * amt_;
	if
	(	amt_ > 2.0 && // If we have less than 3 measurements, we cannot calculate the standard deviation.
		ss_ > 1.0 && // A pair of zero initial measurements will block the addition of other.
		deviation >= 0.0 && // The minimum value is usually closest to the true value.
		sum_square_dev >= K2 * ss_ // Avoids outliers.
	)
		return; // The value is an outlier, do not update the statistics.

	const auto flt_amt = amt_;
	amt_ += n_f;
	const auto total_rev = 1.0 / amt_;
	mean_ = std::fma(mean_, flt_amt, v_f) * total_rev;
	ss_ = std::fma(sum_square_dev, n_f * total_rev, ss_);
	++calls_; // Increment the number of invocations only if the value was added to the statistics.
}

#ifndef NDEBUG
// This code is only compiled in debug mode to test certain library functionality.
namespace
{
	const auto nanotest = []
		{
			static constexpr std::uint64_t samples_simple[] = { 34, 32, 36 }; // Samples that will be added one at a time.
			static constexpr auto M = 2;
			static constexpr std::uint64_t samples_multiple[] = { 34, }; // Samples that will be added M times at once.
			static constexpr std::uint64_t samples_exclude[] = { 1000 }; // Samples that will be excluded from the statistics.
			static constexpr auto exp_flt_cnt = std::size(samples_simple) + M * std::size(samples_multiple); // The total number of samples that will be counted.
			static constexpr auto exp_flt_call = std::size(samples_simple) + std::size(samples_multiple); // The total number of inokes that will be counted.
			static const auto exp_flt_mean = 
				(	std::accumulate(std::cbegin(samples_simple), std::cend(samples_simple), 0.0) +
					M * std::accumulate(std::cbegin(samples_multiple), std::cend(samples_multiple), 0.0)
				) / static_cast<double>(exp_flt_cnt); // The mean value of the samples that will be counted.
			const auto exp_flt_stddev = [] // The standard deviation of the samples that will be counted.
				{	const auto sum_squared_deviations =
					std::accumulate
					(	std::cbegin(samples_simple),
						std::cend(samples_simple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<double>(v) - exp_flt_mean; return std::fma(d, d, i); }
					) +
					M * std::accumulate
					(	std::cbegin(samples_multiple),
						std::cend(samples_multiple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<double>(v) - exp_flt_mean; return std::fma(d, d, i); }
					);
					return std::sqrt(sum_squared_deviations / static_cast<double>(exp_flt_cnt - 1));
				}();

			welford_t md; // Measurement data to be filled in.
			{	
				for (auto x : samples_simple) // Add simple samples one at a time.
				{	md.update(static_cast<double>(x), static_cast<double>(1));
				}
				for (auto x : samples_multiple) // Add multiple samples M times at once.
				{	md.update(static_cast<double>(M * x), static_cast<double>(M));
				}
				for (auto x : samples_exclude) // Add samples that will be excluded from the statistics.
				{	md.update(static_cast<double>(x), static_cast<double>(1));
				}
			}

			assert(md.amt_ == exp_flt_cnt);
			assert(md.calls_ == exp_flt_call);
			assert(std::abs(md.mean_ - exp_flt_mean) / exp_flt_mean < 1e-12);
			const auto s = std::sqrt(md.ss_ / static_cast<double>(md.amt_ - 1U));
			assert(std::abs(s - exp_flt_stddev) / exp_flt_stddev < 1e-12);
			return 0;
		}();
}
#endif // #ifndef NDEBUG

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

#ifndef VI_TIMING_SOURCE_WELFORD_ALG_H
#	define VI_TIMING_SOURCE_WELFORD_ALG_H
#	pragma once

struct welford_t
{
	double mean_ = 0.0; // Mean value.
	double ss_ = 0.0; // Sum of squares.
	double amt_ = 0U; // Number of items processed.
	size_t calls_ = 0U; // Number of invoks processed. Filtered!!!

	void reset() noexcept
	{	mean_ = ss_ = amt_ = 0.0;
		calls_ = 0U;
	}
	void update(double v_f, double n_f) noexcept;
};

#endif // #ifndef VI_TIMING_SOURCE_WELFORD_ALG_H

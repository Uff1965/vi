#ifndef VI_TIMING_SOURCE_INTERNAL_H
#	define VI_TIMING_SOURCE_INTERNAL_H
#	pragma once

#include <chrono>
#include <locale>

#include "build_number_maker.h"

namespace misc
{
	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	constexpr unsigned char PRECISION = 2;
	constexpr unsigned char DEC = 1;

	struct duration_t: std::chrono::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T &&v): std::chrono::duration<double>{ std::forward<T>(v) } {}
		duration_t() = default;

		friend std::string to_string(duration_t d, unsigned char precision, unsigned char dec);

		[[nodiscard]] friend bool operator<(duration_t l, duration_t r)
		{	return l.count() < r.count() && to_string(l, PRECISION, DEC) != to_string(r, PRECISION, DEC);
		}
		template<typename T>
		friend inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const duration_t& d)
		{	return os << to_string(d, PRECISION, DEC);
		}
    };

	std::string to_string(duration_t d, unsigned char precision = PRECISION, unsigned char dec = DEC);

	struct properties_t
	{	misc::duration_t seconds_per_tick_; // [nanoseconds]
		double clock_latency_; // Duration of one clock call [ticks]
		misc::duration_t all_latency_; // Duration of one measurement with preservation. [nanoseconds]
		double clock_resolution_; // [ticks]
		static const properties_t& props();
	private:
		properties_t();
	};
}

#endif // #ifndef VI_TIMING_SOURCE_INTERNAL_H

#ifndef VI_TIMING_SOURCE_INTERNAL_H
#	define VI_TIMING_SOURCE_INTERNAL_H
#	pragma once

#include <chrono>
#include <locale>
#include <string>
#include <string_view>
#include <utility>

namespace misc
{
	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	struct properties_t
	{	std::chrono::duration<double> seconds_per_tick_; // [nanoseconds]
		double clock_latency_; // Duration of one clock call [ticks]
		std::chrono::duration<double> all_latency_; // Duration of one measurement with preservation. [nanoseconds]
		double clock_resolution_; // [ticks]
		static const properties_t& props();
	private:
		properties_t();
	};

	std::string to_string(double d, unsigned char precision, unsigned char dec, std::string_view unit);
}

#endif // #ifndef VI_TIMING_SOURCE_INTERNAL_H

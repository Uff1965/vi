#ifndef VI_TIMING_SOURCE_INTERNAL_H
#	define VI_TIMING_SOURCE_INTERNAL_H
#	pragma once

#include <chrono>
#include <locale> // for std::numpunct
#include <string>
#include <string_view>

namespace misc
{
	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	struct properties_t
	{	std::chrono::duration<double> seconds_per_tick_; // [nanoseconds]
		double clock_latency_ticks_; // Duration of one clock call [ticks]
		std::chrono::duration<double> duration_non_threadsafe_;
		std::chrono::duration<double> duration_ex_threadsafe_;
		std::chrono::duration<double> duration_threadsafe_; // Duration of one measurement with preservation. [nanoseconds]
		double clock_resolution_ticks_; // [ticks]
		static const properties_t &props() { return self_; }
	private:
		properties_t();
		static const properties_t self_;
	};
	inline const properties_t properties_t::self_;

	std::string to_string(double d, unsigned char precision, unsigned char dec);
}

#endif // #ifndef VI_TIMING_SOURCE_INTERNAL_H

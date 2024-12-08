#ifndef VI_TIMING_SOURCE_INTERNAL_H
#	define VI_TIMING_SOURCE_INTERNAL_H
#	pragma once

#include <chrono>
#include <locale>

namespace misc
{
	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	constexpr int PRECISION = 2;
	constexpr int DEC = 1;

	[[nodiscard]] double round_ext(double num, int prec = PRECISION, int dec = DEC, int group = 3);

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
}

struct properties_t
{	misc::duration_t tick_duration_;
	double clock_latency_;
	misc::duration_t all_latency_;
	double clock_resolution_;

private:
	properties_t();
	friend const properties_t& props();
};

const properties_t& props();

#endif // #ifndef VI_TIMING_SOURCE_INTERNAL_H

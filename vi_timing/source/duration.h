#ifndef VI_TIMING_SOURCE_DURATION_H
#	define VI_TIMING_SOURCE_DURATION_H
#	pragma once

#include <chrono>
#include <ostream>
#include <string>
#include <utility>

namespace misc
{
	constexpr unsigned char PRECISION = 2;
	constexpr unsigned char DEC = 1;

	struct duration_t: std::chrono::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T &&v): std::chrono::duration<double>{ std::forward<T>(v) } {}
		duration_t() = default;

		friend std::string to_string(duration_t d, unsigned char precision = PRECISION, unsigned char dec = DEC);

		[[nodiscard]] friend bool operator<(duration_t l, duration_t r)
		{	return l.count() < r.count() && to_string(l, PRECISION, DEC) != to_string(r, PRECISION, DEC);
		}
		template<typename T>
		friend inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const duration_t& d)
		{	return os << to_string(d, PRECISION, DEC);
		}
    };

//	std::string to_string(duration_t d, unsigned char precision = PRECISION, unsigned char dec = DEC);
}
#endif // #ifndef VI_TIMING_SOURCE_DURATION_H

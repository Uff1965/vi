#ifndef VI_TIMING_SOURCE_DURATION_H
#	define VI_TIMING_SOURCE_DURATION_H
#	pragma once

#include <chrono>
#include <ostream>
#include <string>
#include <utility>

namespace misc
{
	using duration = std::chrono::duration<double>;

	std::string to_string(const duration &d, unsigned char precision, unsigned char dec);

	template<unsigned char P = 2, unsigned char D = 1>
	struct duration_t: duration // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T &&v): duration{ std::forward<T>(v) } {}
		duration_t() = default;

		[[nodiscard]] friend std::string to_string(const duration_t &d)
		{	return to_string(d, P, D);
		}
		[[nodiscard]] friend bool operator<(const duration_t &l, const duration_t &r)
		{	return l.count() < r.count() && to_string(l) != to_string(r);
		}
	};
}
#endif // #ifndef VI_TIMING_SOURCE_DURATION_H

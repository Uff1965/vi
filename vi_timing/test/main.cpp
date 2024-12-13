// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// Uncomment the following line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h> // vi_timing interface

#include <chrono>
#include <cmath>
#include <thread>

// Optionally disable optimization.
#if defined(_MSC_VER)
#	define VI_OPTIMIZE_OFF _Pragma("optimize(\"\", off)")
#	define VI_OPTIMIZE_ON  _Pragma("optimize(\"\", on)")
#elif defined(__GNUC__)
#	define VI_OPTIMIZE_OFF _Pragma("GCC push_options") _Pragma("GCC optimize(\"O0\")")
#	define VI_OPTIMIZE_ON  _Pragma("GCC pop_options")
#else
#	define VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_ON
#endif
// Optimizations are disabled to clarity.
VI_OPTIMIZE_OFF

namespace
{
	VI_TM_INIT( vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration); // Initialize the library and request a report at the end of the program.
//	VI_TM_INIT(); // That's also an option.

	int foo()
	{	std::this_thread::sleep_for(std::chrono::milliseconds{ 32 });
		return 0;
	}

	VI_TM("dummy before");
	const auto dummy = foo(); // A global variable that takes some time to initialize.
	VI_TM("dummy after");
}

int main()
{	VI_TM_FUNC; // Timing of function 'main' from this line and to end scope.

	constexpr auto FOO_CNT = 10;
	for (int n = 0; n < FOO_CNT; ++n)
	{	VI_TM("foo");
		foo();
	}
	{	VI_TM("foo series", FOO_CNT);
		for (int n = 0; n < FOO_CNT; ++n)
		{	foo();
		}
	}

	volatile auto result = .0;
	for (auto n = 0; n < 5; ++n)
	{	result = std::sin(n) + std::cos(n);
	}
	constexpr auto MATH_CNT = 1'000;
	for (auto n = 0; n < MATH_CNT; ++n)
	{	VI_TM("math");
		result = std::sin(n) + std::cos(n);
	}
}

// Optimizations are restored.
VI_OPTIMIZE_ON

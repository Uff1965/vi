// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// Uncomment the following line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h> // vi_timing interface

#include <cmath>
#include <cstdio>

namespace
{	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration); // Initialize the library and request a report at the end of the program.
	VI_TM("Global");
}

int main()
{	VI_TM_FUNC; // Timing of function 'main' from this line and to end scope.

	auto foo = [](int n) { return std::sin(n) * std::cos(n); };

	for (int k = 0; k < 1'000; ++k)
	{	constexpr auto CNT = 1'000;
		volatile auto result = .0;

		for (auto n = 0; n < CNT; ++n)
		{	VI_TM("Separately");
			result = foo(n);
		}

		{	VI_TM("Together", CNT);
			for (int n = 0; n < CNT; ++n)
			{	result = foo(n);
			}
		}
	}

	std::puts("Hello, World!");
}
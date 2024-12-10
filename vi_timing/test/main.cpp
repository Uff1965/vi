// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// Uncomment the following line to remove timings.
//#define VI_TM_DISABLE
#include "../vi_timing.h"

#include <cmath>

namespace
{
	double cpu_load()
	{	VI_TM_FUNC;
		volatile double result = .0;
		for (auto n = 0; n < 100; ++n)
		{	result = result + std::sin(n) * std::cos(n);
		}
		return result;
	}

	VI_TM_INIT("Report title:\n", vi_tmSortBySpeed, vi_tmShowResolution); // Initialize the library and request a report at the end of the program.
	VI_TM("Global time"); // That works too!
}

int main()
{	VI_TM_FUNC; // Timing of main function.

	constexpr auto CNT = 100'000;
	volatile auto sum = .0;

	{	VI_TM("series of sth.", CNT); // Measuring a series of something.
		for (int n = 0; n < CNT; ++n)
		{	VI_TM("sth."); // Measuring a something.
			sum = cpu_load(); // something )
		}
	}
}

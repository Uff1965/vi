// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

#include "header.h"

#include <vi/timing.h>

#include <iostream>
#include <cmath>

#if defined(_MSC_VER) && defined(_DEBUG)
const auto _dummy0 = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // To automatically call the _CrtDumpMemoryLeaks function when the program ends
const auto _dummy1 = _set_error_mode(_OUT_TO_MSGBOX); // Error sink is a message box. To be able to ignore errors.
#endif

VI_TM_INIT("Timing report:", reinterpret_cast<vi_tmLogSTR_t>(&std::fputs), stdout, 0xE0);
VI_TM("*** Global ***");

int main()
{
	VI_TM_FUNC;

	vi_tm::warming();

	{
		constexpr auto CNT = 1'000'000;
		volatile int dummy = 0;
		{
			VI_TM("SELF_EXT", CNT);
			for (int n = 0; n < CNT; ++n)
			{
				VI_TM("SELF_INT");
				dummy = 0;
			}
		}

		{
			VI_TM("SELF_EXT_2", CNT * 11);
			for (int n = 0; n < CNT; ++n)
			{
				{ VI_TM("SELF_INT_2"); dummy = 0; }

				{ VI_TM("SELF_INT_2");  dummy = 0; } { VI_TM("SELF_INT_2");  dummy = 0; }
				{ VI_TM("SELF_INT_2");  dummy = 0; } { VI_TM("SELF_INT_2");  dummy = 0; }
				{ VI_TM("SELF_INT_2");  dummy = 0; } { VI_TM("SELF_INT_2");  dummy = 0; }
				{ VI_TM("SELF_INT_2");  dummy = 0; } { VI_TM("SELF_INT_2");  dummy = 0; }
				{ VI_TM("SELF_INT_2");  dummy = 0; } { VI_TM("SELF_INT_2");  dummy = 0; }
			}
		}
	}

	foo();
	std::cout << "Hello World!\n";

	endl(std::cout);

#ifdef VI_TM_DISABLE
	vi_tm::report();
#endif
}

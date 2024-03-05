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

VI_TM_INIT(reinterpret_cast<vi_tmLogSTR>(&std::fputs), stdout, 0x60);
VI_TM("*** Global ***");

int main()
{
	VI_TM_FUNC;

	vi_tm::warming();

	{
		VI_TM("xxx1");

		VI_TM("xxx2");


		VI_TM("xxx3", 1'000'000);
		for (int n = 0; n < 1'000'000; ++n)
		{
			VI_TM("xxx3_inner");
			volatile auto _ = std::sin(n);
		}
	}

	VI_TM_REPORT(reinterpret_cast<vi_tmLogSTR>(&std::fputs), stdout, 0x60);
	VI_TM_REPORT();
	VI_TM_CLEAR;

	foo();
	std::cout << "Hello World!\n";
}

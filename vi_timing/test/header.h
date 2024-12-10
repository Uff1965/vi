#ifndef VI_TIMING_TEST_HEADER_H
#	define VI_TIMING_TEST_HEADER_H
#	pragma once

// For memory leak detection: https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library
#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC //-V2573
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "../vi_timing.h"

#ifdef __cplusplus
extern "C"
{
#endif

	void foo_c(void);
	void bar_c(void);

#ifdef __cplusplus
}
#endif

int global_init();
void run_tests();

#endif // #ifndef VI_TIMING_TEST_HEADER_H

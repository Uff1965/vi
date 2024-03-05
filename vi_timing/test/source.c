// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <vi/timing.h>

void foo(void)
{
	vi_tmAtomicTicks_t* const tm = vi_tmItem("foo", 1);
	const vi_tmTicks_t s = vi_tmGetTicks();

	vi_tmAdd(tm, vi_tmGetTicks() - s);
}

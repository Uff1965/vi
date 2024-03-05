// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi/timing.h>

void foo(void)
{
	viTimingWarming(FALSE, 500);

	viTimingAtomicTicks_t *cnt = viTimingAdd("test C", 1);
	const viTimingTicks_t start = viTimingGetTicks();
	{

	}
	const viTimingTicks_t stop = viTimingGetTicks();
	viTimingIncrement(cnt, stop - start);

	viTimingReport(&fputs, stdout, 0);
	viTimingClear();
}

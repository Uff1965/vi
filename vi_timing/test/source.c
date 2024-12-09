// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../vi_timing.h"

#include <inttypes.h>
#include <stdint.h>
#include <threads.h>

void bar_c(void)
{	const vi_tmTicks_t bar_c_start = vi_tmClock();

	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);
	thrd_yield();

	{	VI_TM_HANDLE htimer = vi_tmCreate();

		vi_tmTicks_t s = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 250000 }, NULL);
		vi_tmFinish(htimer, "xxx", s, 1);
		vi_tmFinish(htimer, "yyy", s, 10);

		puts("Original:");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);

		vi_tmClear(htimer, "xxx");
		puts("After vi_tmClear(htimer, \"xxx\"):");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);

		vi_tmClear(htimer, NULL);
		puts("After vi_tmClear(htimer, NULL):");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);
		vi_tmClose(htimer);
	}

	vi_tmFinish(NULL, "bar_c", bar_c_start, 1);
}

void foo_c(void)
{	const vi_tmTicks_t foo_c_start = vi_tmClock();
	printf("\n%s...\n", __func__); //-V2600
	
	vi_tmWarming(2, 2);
	vi_tmWarming(16, 2);
	
	printf("Version: \'%s\' [%" PRIdPTR "]\n", (const char*)vi_tmInfo(VI_TM_INFO_VERSION), (intptr_t)vi_tmInfo(VI_TM_INFO_VER));

	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 10us", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 100000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 100us", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 1000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 1ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 10ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 14ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 20ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 30ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 1s", start, 1);
	}

	printf("done\n"); //-V2600
	vi_tmFinish(NULL, "foo_c", foo_c_start, 1);
}

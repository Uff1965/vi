// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../timing.h"

#include <inttypes.h>
#include <stdint.h>
#include <threads.h>

void bar_c(void)
{
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);
	thrd_yield();

	{	VI_TM_HITEM h = vi_tmTotalTicks(NULL, "bar", 1);
		const vi_tmTicks_t start = vi_tmGetTicks();

		{
			thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		}

		const vi_tmTicks_t finish = vi_tmGetTicks();
		vi_tmAdd(h, finish - start);
	}

	{
		VI_TM_HANDLE htimer = vi_tmCreate(8);

		VI_TM_HITEM hitem = vi_tmTotalTicks(htimer, "xxx", 1);
		vi_tmTicks_t s = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 250000000 }, NULL);
		vi_tmTicks_t f = vi_tmGetTicks();
		vi_tmAdd(hitem, f - s);
		vi_tmFinish(htimer, "yyy", s, 10);

		vi_tmReport(htimer, (vi_tmLogSTR_t)fputs, stdout, 0);
		puts("");

		vi_tmClear(htimer, "xxx");
		vi_tmReport(htimer, (vi_tmLogSTR_t)fputs, stdout, 0);
		puts("");

		vi_tmClear(htimer, NULL);
		vi_tmReport(htimer, (vi_tmLogSTR_t)fputs, stdout, 0);
		vi_tmClose(htimer);
		puts("");
	}
}

void foo_c(void)
{
	printf("\n%s...\n", __func__); //-V2600
	const vi_tmTicks_t foo_tm = vi_tmGetTicks();

	vi_tmWarming(2, 100);
	vi_tmWarming(16, 100);
	
	printf("Version: \'%s\' [%" PRIdPTR "]\n", (const char*)vi_tmInfo(VI_TM_INFO_VERSION), (intptr_t)vi_tmInfo(VI_TM_INFO_VER));

	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 10us", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 100000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 100us", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 1000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 1ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 10ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 14ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 20ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 30ms", start, 1);
	}

	{
		thrd_yield();
		const vi_tmTicks_t start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL);
		vi_tmFinish(NULL, "thrd_sleep 1s", start, 1);
	}

	vi_tmFinish(NULL, "foo_c", foo_tm, 1);
	printf("done\n"); //-V2600
}

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../vi_timing.h"

#include <inttypes.h>
#include <stdint.h>
#include <threads.h>

static inline void vi_tmFinish(VI_TM_HSHEET h, vi_tmTicks_t start, size_t amount)
{	const vi_tmTicks_t finish = vi_tmClock();
	vi_tmRecord(h, finish - start, amount);
}

void bar_c(void)
{	VI_TM_HSHEET const bar_c_journal = vi_tmSheet(NULL, "bar_c");
	const vi_tmTicks_t bar_c_start = vi_tmClock();

	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);
	thrd_yield();

	{	VI_TM_HBOOK htimer = vi_tmBookCreate();

		VI_TM_HSHEET const j1 = vi_tmSheet(htimer, "xxx");
		VI_TM_HSHEET const j2 = vi_tmSheet(htimer, "yyy");
		vi_tmTicks_t s = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 250000 }, NULL);
		vi_tmFinish(j2, s, 1);
		vi_tmFinish(j1, s, 10);

		puts("Original:");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);

		vi_tmBookClear(htimer, "xxx");
		puts("After vi_tmBookClear(htimer, \"xxx\"):");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);

		vi_tmBookClear(htimer, NULL);
		puts("After vi_tmBookClear(htimer, NULL):");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);
		vi_tmBookClose(htimer);
	}

	vi_tmFinish(bar_c_journal, bar_c_start, 1);
}

void foo_c(void)
{	VI_TM_HSHEET const foo_c_journal = vi_tmSheet(NULL, "foo_c");
	const vi_tmTicks_t foo_c_start = vi_tmClock();
	printf("\n%s...\n", __func__); //-V2600
	
	vi_tmWarming(0, 2);
	vi_tmWarming(2, 2);
	vi_tmWarming(256, 2);
	
	printf("Version: \'%s\' [%" PRIdPTR "]\n", (const char*)vi_tmInfo(VI_TM_INFO_VERSION), (intptr_t)vi_tmInfo(VI_TM_INFO_VER));

	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 10us");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 100us");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 100000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 1ms");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 1000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 10ms");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 14ms");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 20ms");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 30ms");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HSHEET const j = vi_tmSheet(NULL, "thrd_sleep 1s");
		thrd_yield();
		const vi_tmTicks_t start = vi_tmClock();
		thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	printf("done\n"); //-V2600
	vi_tmFinish(foo_c_journal, foo_c_start, 1);
}

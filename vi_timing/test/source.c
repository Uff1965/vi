// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../vi_timing.h"

#include <inttypes.h>
#include <stdint.h>
#include <threads.h>

static inline void vi_tmFinish(VI_TM_HMEASPOINT h, VI_TM_TICK start, size_t amount)
{	const VI_TM_TICK finish = vi_tmGetTicks();
	vi_tmMeasPointAdd(h, finish - start, amount);
}

void bar_c(void)
{	VI_TM_HMEASPOINT const bar_c_journal = vi_tmMeasPoint(NULL, "bar_c");
	const VI_TM_TICK bar_c_start = vi_tmGetTicks();

	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);
	thrd_yield();

	{	VI_TM_HJOURNAL htimer = vi_tmJournalCreate();

		VI_TM_HMEASPOINT const j1 = vi_tmMeasPoint(htimer, "xxx");
		VI_TM_HMEASPOINT const j2 = vi_tmMeasPoint(htimer, "yyy");
		VI_TM_TICK s = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 250000 }, NULL);
		vi_tmFinish(j2, s, 1);
		vi_tmFinish(j1, s, 10);

		puts("Original:");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);

		vi_tmJournalClear(htimer, "xxx");
		puts("After vi_tmJournalClear(htimer, \"xxx\"):");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);

		vi_tmJournalClear(htimer, NULL);
		puts("After vi_tmJournalClear(htimer, NULL):");
		vi_tmReport(htimer, vi_tmShowNoHeader, (vi_tmLogSTR_t)fputs, stdout);
		vi_tmJournalClose(htimer);
	}

	vi_tmFinish(bar_c_journal, bar_c_start, 1);
}

void foo_c(void)
{	VI_TM_HMEASPOINT const foo_c_journal = vi_tmMeasPoint(NULL, "foo_c");
	const VI_TM_TICK foo_c_start = vi_tmGetTicks();
	printf("\n%s:\n", __func__); //-V2600
	
	vi_tmWarming(256, 2);
	
	printf("Version: \'%s\' [%" PRIdPTR "]\n", (const char*)vi_tmInfo(VI_TM_INFO_VERSION), (intptr_t)vi_tmInfo(VI_TM_INFO_VER));

	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 10us");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 100us");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 100000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 1ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 1000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 10ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 14ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 20ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 30ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	{	VI_TM_HMEASPOINT const j = vi_tmMeasPoint(NULL, "thrd_sleep 1s");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL);
		vi_tmFinish(j, start, 1);
	}

	printf("%s - done\n", __func__); //-V2600
	vi_tmFinish(foo_c_journal, foo_c_start, 1);
}

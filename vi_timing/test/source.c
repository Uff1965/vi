// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../vi_timing.h"

#include <inttypes.h>
#include <stdint.h>
#include <threads.h>

static inline void vi_tmFinish(VI_TM_HMEAS measure, VI_TM_TICK start, size_t amount)
{	const VI_TM_TICK finish = vi_tmGetTicks();
	vi_tmMeasuringAdd(measure, finish - start, amount);
}

void bar_c(void)
{	VI_TM_HMEAS const bar_c_measuring = vi_tmMeasuring(NULL, "bar_c");
	const VI_TM_TICK bar_c_start = vi_tmGetTicks();

	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);
	thrd_yield();

	{	VI_TM_HJOUR journal = vi_tmJournalCreate();

		VI_TM_HMEAS const m1 = vi_tmMeasuring(journal, "xxx");
		VI_TM_HMEAS const m2 = vi_tmMeasuring(journal, "yyy");
		VI_TM_TICK s = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 250000 }, NULL);
		vi_tmFinish(m2, s, 1);
		vi_tmFinish(m1, s, 10);

		puts("Original:");
		vi_tmReport(journal, vi_tmShowNoHeader, (vi_tmRptCb_t)fputs, stdout);

		vi_tmMeasuringClear(m1);
		puts("After vi_tmMeasuringClear(m1) (Clear \"xxx\"):");
		vi_tmReport(journal, vi_tmShowNoHeader, (vi_tmRptCb_t)fputs, stdout);

		vi_tmJournalClear(journal, NULL);
		puts("After vi_tmJournalClear(htimer, NULL):");
		vi_tmReport(journal, vi_tmShowNoHeader, (vi_tmRptCb_t)fputs, stdout);
		vi_tmJournalClose(journal);
	}

	vi_tmFinish(bar_c_measuring, bar_c_start, 1);
}

void foo_c(void)
{	VI_TM_HMEAS const foo_c_measuring = vi_tmMeasuring(NULL, "foo_c");
	const VI_TM_TICK foo_c_start = vi_tmGetTicks();
	printf("\n%s:\n", __func__); //-V2600
	
	vi_tmWarming(256, 2);
	
	printf("Version: \'%s\' [%" PRIdPTR "]\n", (const char*)vi_tmInfo(VI_TM_INFO_VERSION), (intptr_t)vi_tmInfo(VI_TM_INFO_VER));

	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 10us");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 100us");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 100000 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 1ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 1000000 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 10ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 14ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 20ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 30ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasuring(NULL, "thrd_sleep 1s");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL);
		vi_tmFinish(meas, start, 1);
	}

	printf("%s - done\n", __func__); //-V2600
	vi_tmFinish(foo_c_measuring, foo_c_start, 1);
}

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "vi_timing/vi_timing_proxy.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h> // For fputs and stdout
#include <threads.h>

// Consider inspecting the first argument of the 'vi_tmMeasurementAdd' function call.

static inline void finish(VI_TM_HMEAS measure, VI_TM_TICK start, VI_TM_SIZE cnt)
{	const VI_TM_TICK finish = vi_tmGetTicks();
	vi_tmMeasurementAdd(measure, finish - start, cnt);
}

void bar_c(void)
{	VI_TM_HMEAS const bar_c_measuring = vi_tmMeasurement(VI_TM_HGLOBAL, "bar_c");
	const VI_TM_TICK bar_c_start = vi_tmGetTicks();

	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);
	thrd_yield();

	{	VI_TM_HJOUR journal = vi_tmJournalCreate(0, NULL);

		VI_TM_HMEAS const m1 = vi_tmMeasurement(journal, "xxx");
		VI_TM_HMEAS const m2 = vi_tmMeasurement(journal, "yyy");
		VI_TM_TICK s = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 250000 }, NULL);
		finish(m2, s, 1);
		finish(m1, s, 10);

		puts("Original:");
		vi_tmReport(journal, (unsigned)vi_tmHideHeader, (vi_tmReportCb_t)fputs, stdout);

		vi_tmMeasurementReset(m1);
		puts("After vi_tmMeasurementReset(m1) (Clear \"xxx\"):");
		vi_tmReport(journal, (unsigned)vi_tmHideHeader, NULL, NULL);

		vi_tmJournalReset(journal);
		puts("After vi_tmJournalReset(htimer):");
		vi_tmReport(journal, (unsigned)vi_tmHideHeader, NULL, NULL);
		vi_tmJournalClose(journal);
	}

	finish(bar_c_measuring, bar_c_start, 1);
}

void foo_c(void)
{	VI_TM_HMEAS const foo_c_measuring = vi_tmMeasurement(VI_TM_HGLOBAL, "foo_c");
	const VI_TM_TICK foo_c_start = vi_tmGetTicks();
	printf("\n%s:\n", __func__);
	
	vi_Warming(256, 2);
	
	printf("Version: \'%s\' [%u]\n", (const char*)vi_tmStaticInfo(VI_TM_INFO_VERSION), *(const unsigned*)vi_tmStaticInfo(VI_TM_INFO_VER));
	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 10us");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000 }, NULL);
		finish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 100us");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 100000 }, NULL);
		finish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 1ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 1000000 }, NULL);
		finish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 10ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);
		finish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 14ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);
		finish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 20ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);
		finish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 30ms");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		finish(meas, start, 1);
	}

	{	VI_TM_HMEAS const meas = vi_tmMeasurement(VI_TM_HGLOBAL, "thrd_sleep 1s");
		thrd_yield();
		const VI_TM_TICK start = vi_tmGetTicks();
		thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL);
		finish(meas, start, 1);
	}

	printf("%s - done\n", __func__);
	finish(foo_c_measuring, foo_c_start, 1);
}

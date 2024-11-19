// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../timing.h"

#include <inttypes.h>
#include <stdint.h>
#include <threads.h>

//VI_OPTIMIZE_OFF
void bar_c(void)
{
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);
	thrd_yield();

	{	const vi_tmTicks_t start = vi_tmGetTicks();

		{
			thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		}

		const vi_tmTicks_t finish = vi_tmGetTicks();
		vi_tmAdd(vi_tmTotalTicks(NULL, "bar", 1), finish - start);
	}
}
//VI_OPTIMIZE_ON

//VI_OPTIMIZE_OFF
void foo_c(void)
{
	vi_tmWarming(2, 100);
	vi_tmWarming(16, 100);
	
	printf("Version: \'%s\' [%" PRIdPTR "]\n", (const char*)vi_tmInfo(VI_TM_INFO_VERSION), (intptr_t)vi_tmInfo(VI_TM_INFO_VER));

	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	const vi_tmTicks_t foo_tm = vi_tmGetTicks();

	printf("\n%s... ", __func__); //-V2600

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
//VI_OPTIMIZE_ON

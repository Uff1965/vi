// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../timing.h"

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
		vi_tmAdd(vi_tmItem("bar", 1), finish - start);
	}
}
//VI_OPTIMIZE_ON

//VI_OPTIMIZE_OFF
void foo_c(void)
{
	vi_tmWarming(0, 100);

	printf("Version: \'%s\' [%d]\n", vi_tmInfo(VI_TM_INFO_VERSION), vi_tmInfo(VI_TM_INFO_VER));

	thrd_yield();
	thrd_sleep(&(struct timespec) { .tv_nsec = 100 }, NULL);

	bar_c();

	struct vi_tmItem_t const foo_tm = vi_tmStart("foo_c", 1);

	printf("\n%s... ", __func__); //-V2600

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 10us", 1);
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000 }, NULL);
		vi_tmFinish(&tm);
	}

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 100us", 1);
		thrd_sleep(&(struct timespec) { .tv_nsec = 100000 }, NULL);
		vi_tmFinish(&tm);
	}

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 1ms", 1);
		thrd_sleep(&(struct timespec) { .tv_nsec = 1000000 }, NULL);
		vi_tmFinish(&tm);
	}

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 10ms", 1);
		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);
		vi_tmFinish(&tm);
	}

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 14ms", 1);
		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);
		vi_tmFinish(&tm);
	}

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 20ms", 1);
		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);
		vi_tmFinish(&tm);
	}

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 30ms", 1);
		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);
		vi_tmFinish(&tm);
	}

	{
		thrd_yield();
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 1s", 1);
		thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL);
		vi_tmFinish(&tm);
	}

	vi_tmFinish(&foo_tm);
	printf("done\n"); //-V2600
}
//VI_OPTIMIZE_ON

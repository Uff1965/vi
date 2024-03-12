// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <vi/timing.h>

#include <threads.h>

//VI_OPTIMIZE_OFF
void foo(void)
{
	struct vi_tmItem_t const foo_tm = vi_tmStart("foo", 1);

	{
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 10ms", 1);

		thrd_sleep(&(struct timespec) { .tv_nsec = 10000000 }, NULL);

		vi_tmEnd(&tm);
	}

	{
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 14ms", 1);

		thrd_sleep(&(struct timespec) { .tv_nsec = 14000000 }, NULL);

		vi_tmEnd(&tm);
	}

	{
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 20ms", 1);

		thrd_sleep(&(struct timespec) { .tv_nsec = 20000000 }, NULL);

		vi_tmEnd(&tm);
	}

	{
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 30ms", 1);

		thrd_sleep(&(struct timespec) { .tv_nsec = 30000000 }, NULL);

		vi_tmEnd(&tm);
	}

	{
		struct vi_tmItem_t const tm = vi_tmStart("thrd_sleep 10s", 1);

		thrd_sleep(&(struct timespec) { .tv_sec = 10 }, NULL);

		vi_tmEnd(&tm);
	}

	vi_tmEnd(&foo_tm);
}
//VI_OPTIMIZE_ON

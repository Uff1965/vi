// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <vi/timing.h>

#include <threads.h>

//VI_OPTIMIZE_OFF
void foo(void)
{
	{
		vi_tmAtomicTicks_t* const tm = vi_tmItem("foo", 1);
		const vi_tmTicks_t s = vi_tmGetTicks();
		vi_tmAdd(tm, s);
	}

	{
		vi_tmAtomicTicks_t* const tm = vi_tmItem("thrd_sleep 10ms", 1);
		const vi_tmTicks_t s = vi_tmGetTicks();

		thrd_sleep(&(struct timespec) { .tv_nsec = 10'000'000 }, NULL);

		vi_tmAdd(tm, s);
	}

	{
		vi_tmAtomicTicks_t* const tm = vi_tmItem("thrd_sleep 14ms", 1);
		const vi_tmTicks_t s = vi_tmGetTicks();

		thrd_sleep(&(struct timespec) { .tv_nsec = 14'000'000 }, NULL);

		vi_tmAdd(tm, s);
	}

	{
		vi_tmAtomicTicks_t* const tm = vi_tmItem("thrd_sleep 20ms", 1);
		const vi_tmTicks_t s = vi_tmGetTicks();

		thrd_sleep(&(struct timespec) { .tv_nsec = 20'000'000 }, NULL);

		vi_tmAdd(tm, s);
	}

	{
		vi_tmAtomicTicks_t* const tm = vi_tmItem("thrd_sleep 30ms", 1);
		const vi_tmTicks_t s = vi_tmGetTicks();

		thrd_sleep(&(struct timespec) { .tv_nsec = 30'000'000 }, NULL);

		vi_tmAdd(tm, s);
	}

	{
		vi_tmAtomicTicks_t* const tm = vi_tmItem("thrd_sleep 10s", 1);
		const vi_tmTicks_t s = vi_tmGetTicks();

		thrd_sleep(&(struct timespec) { .tv_sec = 10 }, NULL);

		vi_tmAdd(tm, s);
	}
}
//VI_OPTIMIZE_ON
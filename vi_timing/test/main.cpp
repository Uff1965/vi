//#define VI_TM_DISABLE // Uncomment this line to remove timing.
#include <vi_timing/vi_timing.h>
#include <atomic>

namespace
{
	VI_TM_INIT(vi_tmSortByAverage, vi_tmShowResolution, vi_tmShowDuration);

	VI_TM("CNT"); // Lifespan of the CNT variable.
	constexpr int CNT = 10;
	constexpr char GROUP[] = "Group";
	constexpr char NOTHING[] = "Nothing";
}

VI_OPTIMIZE_OFF
int main()
{	VI_TM_FUNC;

	{	// Optional.
		vi_tmThreadPrepare(); // Fix the CPU affinity for the current thread, warms up the CPU and waits for the next time quantum to start.
		VI_TM_CLEAR(GROUP);
		VI_TM_CLEAR(NOTHING);
	}

	{	VI_TM(GROUP, CNT);
		for (int n = 0; n < CNT; ++n)
		{	VI_TM(NOTHING);
		}
	}

	{
		constexpr auto N = 10'000U;

		{	int i = 0;
			VI_TM("int", N);
			for (unsigned n = 0; n < N; ++n)
			{	i = 777;
			}
		}

		{	VI_TM("Empty", N);
			for (unsigned n = 0; n < N; ++n)
			{/**/}
		}

		{	volatile int i = 0;
			VI_TM("volatile", N);
			for (unsigned n = 0; n < N; ++n)
			{
				i = 777;
			}
		}

		std::atomic<int> i = 0;
		{	VI_TM("atomic", N);
			for (unsigned n = 0; n < N; ++n)
			{
				i = 777;
			}
		}

		{	VI_TM("seq_cst", N);
			for (unsigned n = 0; n < N; ++n)
			{
				i.store(777, std::memory_order_seq_cst);
			}
		}

		{	VI_TM("relaxed", N);
			for (unsigned n = 0; n < N; ++n)
			{
				i.store(777, std::memory_order_relaxed);
			}
		}

		{	VI_TM("release", N);
			for (unsigned n = 0; n < N; ++n)
			{
				i.store(777, std::memory_order_release);
			}
		}
	}

	vi_tmThreadAffinityRestore();

	puts("Hello, World!\n");
}
VI_OPTIMIZE_ON

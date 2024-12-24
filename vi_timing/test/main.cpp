// Uncomment the next line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h>

#include <atomic>

namespace
{	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration);
	VI_TM("GLOBAL");

	void burden(int cnt)
	{	static std::atomic<unsigned> dummy = 0U;
		for (auto n = 0; n < cnt; ++n)
			++dummy;
	};

	template<std::memory_order M>
	void burden(int cnt)
	{	static std::atomic<unsigned> dummy = 0U;
		for (auto n = 0; n < cnt; ++n)
			dummy.fetch_add(1, M);
	};
}

int main(int argn, char* args[])
{	VI_TM_FUNC;
	printf("Version of \'vi_timing\' library: %s\n\n", VI_TM_FULLVERSION);
	
	static const auto cnt = argn == 2 ? std::atoi(args[1]) : 1'000;
	printf("Burden: %d\n\n", cnt);

	constexpr auto CNT = 1'000;
	for (int m = 0; m < 100; ++m)
	{
		{
			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE volatile");
				burden(cnt);
			}

			{	VI_TM("GROUP volatile", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden(cnt);
				}
			}
		}

		{
			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE seq_cst");
				burden<std::memory_order_seq_cst>(cnt);
			}

			{	VI_TM("GROUP seq_cst", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden<std::memory_order_seq_cst>(cnt);
				}
			}
		}

		{
			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE relaxed");
				burden<std::memory_order_relaxed>(cnt);
			}

			{	VI_TM("GROUP relaxed", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden<std::memory_order_relaxed>(cnt);
				}
			}
		}

		{
			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE release");
				burden<std::memory_order_release>(cnt);
			}

			{	VI_TM("GROUP release", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden<std::memory_order_release>(cnt);
				}
			}
		}

		{
			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE acquire");
				burden<std::memory_order_acquire>(cnt);
			}

			{	VI_TM("GROUP acquire", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden<std::memory_order_acquire>(cnt);
				}
			}
		}

		{
			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE acq_rel");
				burden<std::memory_order_acq_rel>(cnt);
			}

			{	VI_TM("GROUP acq_rel", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden<std::memory_order_acq_rel>(cnt);
				}
			}
		}

		{
			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE consume");
				burden<std::memory_order_consume>(cnt);
			}

			{	VI_TM("GROUP consume", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden<std::memory_order_consume>(cnt);
				}
			}
		}
	}
}

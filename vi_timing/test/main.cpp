// Uncomment the next line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h>

#include <atomic>

namespace
{	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration);
	VI_TM("GLOBAL");
}

int main(int argn, char* args[])
{	VI_TM_FUNC;
	printf("Version of \'vi_timing\' library: %s\n\n", VI_TM_FULLVERSION);
	
	static const auto cnt = argn == 2 ? std::atoi(args[1]) : 1'000;
	printf("Burden: %d\n\n", cnt);

	constexpr auto CNT = 1'000;
	for (int m = 0; m < 100; ++m)
	{
		{	static volatile auto dummy = 0U;
			auto burden = []()
				{	for (auto n = cnt; n; --n)
					++dummy;
				};

			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE volatile");
				burden();
			}

			{	VI_TM("GROUP volatile", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden();
				}
			}
		}

		{	static std::atomic<unsigned> dummy = 0U;
			auto burden = []()
				{	for (auto n = cnt; n; --n)
						dummy.fetch_add(1, std::memory_order_seq_cst);
				};

			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE seq_cst");
				burden();
			}

			{	VI_TM("GROUP seq_cst", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden();
				}
			}
		}

		{	static std::atomic<unsigned> dummy = 0U;
			auto burden = []()
				{	for (auto n = cnt; n; --n)
						dummy.fetch_add(1, std::memory_order_relaxed);
				};

			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE relaxed");
				burden();
			}

			{	VI_TM("GROUP relaxed", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden();
				}
			}
		}

		{	static std::atomic<unsigned> dummy = 0U;
			auto burden = []()
				{	for (auto n = cnt; n; --n)
						dummy.fetch_add(1, std::memory_order_release);
				};

			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE release");
				burden();
			}

			{	VI_TM("GROUP release", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden();
				}
			}
		}

		{	static std::atomic<unsigned> dummy = 0U;
			auto burden = []()
				{	for (auto n = cnt; n; --n)
						dummy.fetch_add(1, std::memory_order_acquire);
				};

			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE acquire");
				burden();
			}

			{	VI_TM("GROUP acquire", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden();
				}
			}
		}

		{	static std::atomic<unsigned> dummy = 0U;
			auto burden = []()
				{	for (auto n = cnt; n; --n)
						dummy.fetch_add(1, std::memory_order_acq_rel);
				};

			for (auto n = 0; n < CNT; ++n)
			{	VI_TM("ALONE acq_rel");
				burden();
			}

			{	VI_TM("GROUP acq_rel", CNT);
				for (int n = 0; n < CNT; ++n)
				{	burden();
				}
			}
		}
	}
}

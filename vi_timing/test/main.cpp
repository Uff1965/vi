// Uncomment the next line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h>

#include <thread>

namespace
{	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration);
	VI_TM("GLOBAL");
}

int main(int argn, char* args[])
{	VI_TM_FUNC;
	constexpr auto CNT = 1'000;
	printf("Version of \'vi_timing\' library: %s\n\n", VI_TM_FULLVERSION);
	
	static const auto cnt = argn > 1 ? std::atoi(args[1]) : 1'000;
	printf("Burden: %d\n\n", cnt);

	static volatile auto dummy = 0U;
	auto burden = []()
		{	for (auto n = cnt; n; --n)
				++dummy;
		};

	for (int m = 0; m < 1'000; ++m)
	{
		for (auto n = 0; n < CNT; ++n)
		{	VI_TM("ALONE");
			burden();
		}

		{	VI_TM("JOINTLY", CNT);
			for (int n = 0; n < CNT; ++n)
			{	burden();
			}
		}
	}
}

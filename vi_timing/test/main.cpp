// Uncomment the next line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h>

namespace
{	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration);
	VI_TM("Global");
}

int main(int argn, char* args[])
{	VI_TM_FUNC;
	constexpr auto CNT = 10'000;
	printf("Version of \'vi_timing\' library: %s\n\n", VI_TM_FULLVERSION);
	
	static const auto cnt = argn > 1 ? std::atoi(args[1]) : 10'000;
	printf("n = %d\n\n", cnt);

	static volatile auto dummy = 0U;
	auto foo = []()
		{	for (auto n = cnt; n--; )
				++dummy;
		};

	for (auto n = 0; n < CNT; ++n)
	{	VI_TM("Separately");
		foo();
	}

	{	VI_TM("Together", CNT);
		for (int n = 0; n < CNT; ++n)
		{	foo();
		}
	}
}

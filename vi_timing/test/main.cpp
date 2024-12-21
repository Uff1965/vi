// Uncomment the following line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h>

namespace
{	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration);
	VI_TM("Global");
}

int main()
{	VI_TM_FUNC;
	constexpr auto CNT = 10'000;
	
	auto foo = []()
		{	volatile auto dummy = 0U;
			for (auto n = 10'000; n--; )
				++dummy;
		};

	{
		for (auto n = 0; n < CNT; ++n)
		{	VI_TM("Separately");
			foo();
		}
	}

	{
		VI_TM("Together", CNT);
		for (int n = 0; n < CNT; ++n)
		{	foo();
		}
	}
}


// Uncomment the next line to remove timing.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h>

namespace
{
	void load(int cnt)
	{	static volatile auto dummy = 0U;
		for (auto n = cnt; n; --n)
		{	++dummy;
		}
	}

	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration);
}

int main(int argn, char *args[])
{	VI_TM_FUNC;

	constexpr int CNT = 100;
	const auto amt = argn == 2 ? std::atoi(args[1]) : 500'000;
	const auto amt100 = amt * 100;

	printf("amt = %d\n", amt);

	vi_tmThreadAffinityFixate();
	vi_tmWarming();
	vi_tmThreadYield();
	for(int n = 0; n < 5; ++n)
	{	VI_TM("");
		load(amt);
	}

	for (int n = 0; n < CNT; ++n)
	{	VI_TM("load*100");
		load(amt100);
	}

	{	VI_TM("load group", CNT);
		for (int n = 0; n < CNT; ++n)
		{	load(amt);
		}
	}

	{	VI_TM("load");
		load(amt);
	}

	puts("Hello, World!\n");
}

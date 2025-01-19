//#define VI_TM_DISABLE // Uncomment the next line to remove timing.
#include <vi_timing/vi_timing.h>

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

	vi_tmThreadPrepare();
	VI_TM_CLEAR(GROUP);
	VI_TM_CLEAR(NOTHING);
	for (int n = 0; n < 5; ++n)
	{	VI_TM("");
	}

	{	VI_TM(GROUP, CNT);
		for (int n = 0; n < CNT; ++n)
		{	VI_TM(NOTHING);
		}
	}
	vi_tmThreadAffinityRestore();

	puts("Hello, World!\n");
}
VI_OPTIMIZE_ON

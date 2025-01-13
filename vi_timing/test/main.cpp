// Uncomment the next line to remove timings.
//#define VI_TM_DISABLE
#include <vi_timing/vi_timing.h>

namespace
{	VI_TM_INIT(vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowDuration);
	VI_TM("GLOBAL");
}

int main(int argn, char* args[])
{	VI_TM_FUNC;
	printf("Version of \'vi_timing\' library: %s\n\n", VI_TM_FULLVERSION);
	
	const auto cnt1 = argn == 2 ? std::atoi(args[1]) : 50;
	const auto cnt10 = 10 * cnt1;
	printf("Burden: %d\n\n", cnt1);

	auto burden = [](int cnt)
		{	static volatile auto dummy = 0U;
			for (auto n = cnt; n; --n)
				++dummy;
		};

	for (int m = 0; m < 100; ++m)
	{	constexpr auto CNT = 1'000;

		for (auto n = 0; n < CNT; ++n)
		{	VI_TM("ALONE");
			burden(cnt1);
		}

		for (auto n = 0; n < CNT; ++n)
		{	VI_TM("ALONE 10");
			burden(cnt10);
		}

		{	VI_TM("GROUP", CNT);
			for (int n = 0; n < CNT; ++n)
			{	burden(cnt1);
			}
		}

		{	VI_TM("GROUP 10", CNT);
			for (int n = 0; n < CNT; ++n)
			{	burden(cnt10);
			}
		}
	}

	const double *overhead = reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_OVERHEAD));
	const double *unit = reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_UNIT));
	printf("Overhead: %.4g\nUnit: %.4g sec.\n", *overhead, *unit);

	auto fn = [](const char *name, VI_TM_TICK total, std::size_t amount, std::size_t calls_cnt, void *)
		{	printf("%10s: total = %10zd, amount = %6zd, calls = %6zd\n", name, total, amount, calls_cnt);
			return 0;
		};
	vi_tmResults(nullptr, fn, nullptr);
	puts("");
}

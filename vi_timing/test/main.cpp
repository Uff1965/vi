// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC //-V2573
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include "../vi_timing.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

//#undef VI_OPTIMIZE_ON
//#define VI_OPTIMIZE_ON
//#undef VI_OPTIMIZE_OFF
//#define VI_OPTIMIZE_OFF

using namespace std::literals;
namespace ch = std::chrono;

namespace
{
#ifdef NDEBUG
	constexpr char CONFIG[] = "Release";
#else
	constexpr char CONFIG[] = "Debug";
#endif

	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	VI_TM_INIT(vi_tmSortBySpeed, "Global timing report:\n", vi_tmShowDuration, vi_tmShowOverhead, vi_tmShowUnit, vi_tmShowResolution);
	VI_TM("GLOBAL");

#if defined(_MSC_VER) && defined(_DEBUG)
	const auto init_win_debug = []
		{	VI_TM("Initialize WinDebug");
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // To automatically call the _CrtDumpMemoryLeaks function when the program ends
			_set_error_mode(_OUT_TO_MSGBOX); // Error sink is a message box. To be able to ignore errors.
			return 0;
		}();
#endif
	const auto init_common = []
		{	VI_TM("INITIALIZE COMMON");
//			vi_tmThreadAffinityFixate();
			return 0;
		}();
}

namespace {
VI_OPTIMIZE_OFF
	void test_multithreaded()
	{	VI_TM_FUNC;
		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOURNAL>, decltype(&vi_tmJournalClose)> h{ vi_tmJournalCreate(), &vi_tmJournalClose };

#ifdef NDEBUG
		static constexpr auto CNT = 2'000'000;
#else
		static constexpr auto CNT = 100'000;
#endif
		static std::atomic<std::size_t> v{};

		std::cout << "\nTest multithreaded():" << std::endl;

		auto load = [h = h.get()]
		{	static auto j_load = vi_tmMeasPoint(h, "load");
			vi_tm::meter_t tm{ j_load };

			for (auto n = CNT; n; --n)
			{	const auto s = vi_tmGetTicks();
				const auto f = vi_tmGetTicks();
				const auto name = "check_" + std::to_string(n % 4); //-V112 "Dangerous magic number 4 used"
				vi_tmMeasPointAdd(vi_tmMeasPoint(h, name.c_str()), f - s, 1);
				v++;
			}
		};

		std::vector<std::thread> threads{ 2 * std::thread::hardware_concurrency() };
		for (auto& t : threads)
		{	t = std::thread{ load };
		}

		for (auto& t : threads)
		{	t.join();
		}

		if (v.load() != CNT * threads.size())
		{	std::cerr << "******* ERROR !!! *************\n";
			assert(false);
		}

		std::cout << "v: " << v << " [" << CNT << "*" << threads.size() << "]" << "\n";
		std::cout << "Timing:\n";
		vi_tmReport(h.get(), vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit | vi_tmShowResolution);
		std::cout << "nTest multithreaded - done" << std::endl;
	}
VI_OPTIMIZE_ON

	void test_instances()
	{	VI_TM("Test additional timers");
		std::cout << "\nAdditional timers:\n";
		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOURNAL>, decltype(&vi_tmJournalClose)> handler{ vi_tmJournalCreate(), &vi_tmJournalClose };
		{	auto h = handler.get();
			{	static auto j1 = vi_tmMeasPoint(h, "long, long, long, very long name");
				vi_tm::meter_t tm1{ j1 };
				{	static auto j2 = vi_tmMeasPoint(h, "100ms * 10");
					vi_tm::meter_t tm2{ j2, 10 };
					for (int n = 0; n < 10; ++n)
					{	std::this_thread::sleep_for(100ms);

						static auto j3 = vi_tmMeasPoint(h, "tm");
						vi_tm::meter_t tm3{ j3 };
						static auto j4 = vi_tmMeasPoint(h, "tm_empty");
						vi_tm::meter_t tm4{ j4 };
					}
				}
			}
			vi_tmReport(h, vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit);
		}
		std::cout << "nTest additional timers - done" << std::endl;
	}

VI_OPTIMIZE_OFF
	void test_empty()
	{	VI_TM_FUNC;
		std::cout << "\nTest test_empty:\n";

		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOURNAL>, decltype(&vi_tmJournalClose)> handler{ vi_tmJournalCreate(), &vi_tmJournalClose };
		{	auto const h = handler.get();

			vi_tmJournalClear(h, "vi_tm");
			vi_tmJournalClear(h, "empty");
			VI_TM_CLEAR("VI_TM");
			VI_TM_CLEAR("EMPTY");

			static auto const jTm = vi_tmMeasPoint(h, "vi_tm");
			static auto const jEmpty = vi_tmMeasPoint(h, "empty");

			for (int n = 0; n < 1'000'000; ++n)
			{
				{
					const auto sTm = vi_tmGetTicks();
					const auto sEmpty = vi_tmGetTicks(); //-V656 Variables 'sTm', 'sEmpty' are initialized through the call to the same function.
					/**/
					const auto fEmpty = vi_tmGetTicks();
					vi_tmMeasPointAdd(jEmpty, fEmpty - sEmpty, 1);
					const auto fTm = vi_tmGetTicks();
					vi_tmMeasPointAdd(jTm, fTm - sTm, 1);
				}

				{	VI_TM("VI_TM");
					VI_TM("EMPTY");
				}
			}

			const char *name = nullptr;
			VI_TM_TICKDIFF ticks = 0U;
			std::size_t amount = 0U;
			std::size_t calls_cnt = 0U;
			vi_tmMeasPointGet(vi_tmMeasPoint(h, "vi_tm"), &name, &ticks, &amount, &calls_cnt);
			std::cout << "vi_tm:\tticks = " << std::setw(16) << ticks << ",\tamount = " << amount << ",\tcalls = " << calls_cnt << std::endl;
			vi_tmMeasPointGet(vi_tmMeasPoint(h, "empty"), &name, &ticks, &amount, &calls_cnt);
			std::cout << "empty:\tticks = " << std::setw(16)  << ticks << ",\tamount = " << amount << ",\tcalls = " << calls_cnt << std::endl;
			endl(std::cout);
			vi_tmReport(h, vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit | vi_tmShowResolution);
		}

		std::cout << "Test test_empty - Done" << std::endl;
	}
VI_OPTIMIZE_ON

	void prn_header()
	{	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());

		std::cout <<
			"\n"
	#pragma warning(suppress: 4996)
			"Start: " << std::put_time(std::localtime(&tm), "%F %T.\n") <<
			"Build type: " << CONFIG << "\n" <<
			std::endl;

		std::cout <<
			"Information about the \'vi_timing\' library:" << "\n"
			"\tBuild type: " << reinterpret_cast<const char*>(vi_tmInfo(VI_TM_INFO_BUILDTYPE)) << "\n"
			"\tVer: " << vi_tmInfo(VI_TM_INFO_VER) << "\n"
			"\tBuild number: " << vi_tmInfo(VI_TM_INFO_BUILDNUMBER) << "\n"
			"\tVersion: " << VI_TM_FULLVERSION << std::endl;
	}

	void warming()
	{	endl(std::cout);
		std::cout << "Warming... ";
		{	VI_TM("Warming in main()");
			vi_tmWarming(1);
		}
		assert(0 == errno);
		std::cout << "done" << std::endl;
	}

	void prn_clock_properties()
	{	std::cout << "\nClock properties:";
		if (auto ptr = reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_RESOLUTION)); ptr)
		{	std::cout << "\nResolution: " << std::setprecision(3) << *ptr << " ticks";
		}
		if (auto ptr = reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_DURATION)); ptr)
		{	std::cout << "\nDuration: " << std::setprecision(3) << 1e9 * *ptr << " ns.";
		}
		if (auto ptr = reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_OVERHEAD)); ptr)
		{	std::cout << "\nAdditive: " << std::setprecision(3) << *ptr << " ticks";
		}
		if (auto ptr = reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_UNIT)); ptr)
		{	std::cout << "\nTick: " << std::setprecision(3) << 1e9 * *ptr << " ns.";
		}
		std::cout << "\nClock properties - done" << std::endl;
	}

	void test_vi_tmResults()
	{
		struct props_t
		{	double add_;
			double unit_;
		} const props =
		{	*reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_OVERHEAD)),
			*reinterpret_cast<const double *>(vi_tmInfo(VI_TM_INFO_UNIT))
		};

		std::cout << "\nTest vi_tmResults:";
		auto results_callback = [](VI_TM_HMEASPOINT m, void* data)
			{	const char *name;
				VI_TM_TICKDIFF ticks = 0U;
				VI_TM_CNT amount = 0U;
				VI_TM_CNT calls_cnt = 0U;
				vi_tmMeasPointGet(m, &name, &ticks, &amount, &calls_cnt);
				if (0U != calls_cnt)
				{	const auto props = *static_cast<props_t *>(data);
					std::cout << 
					"\n"<< std::left << std::setw(48) << name << ":"
					" ticks = " << std::setw(15) << std::right << ticks << ","
					" (" << std::setprecision(2) << std::setw(9) << std::scientific << (props.unit_ * (ticks - calls_cnt * props.add_) / calls_cnt) << "),"
					" amount = " << std::setw(12) << std::right << amount << ","
					" calls = " << std::setw(12) << std::right << calls_cnt;
				}
				return 0;
			};
		vi_tmMeasPointEnum(nullptr, results_callback, const_cast<props_t*>(&props));
		std::cout << "\nTest vi_tmResults - done" << std::endl;
	}

VI_OPTIMIZE_OFF
	void test()
	{	std::cout << "\nTest test:";
		
		auto nothing = [] {/**/};
		constexpr auto CNT = 10'000;

		vi_tmThreadPrepare();
		{	VI_TM("Test");

			for (auto n = 0; n < 100; ++n)
			{	(void) vi_tmGetTicks();
			}

			{	VI_TM("TestA First");
				VI_TM("TestA");
				nothing();
			}
			{	VI_TM("TestA Other", CNT);
				for (auto n = 0; n < CNT; ++n)
				{	VI_TM("TestA");
					nothing();
				}
			}

			{	VI_TM("TestB First");
				const auto start = vi_tmGetTicks();
				nothing();
				const auto finish = vi_tmGetTicks();
				vi_tmMeasPointAdd(vi_tmMeasPoint(nullptr, "TestB"), finish - start);
			}
			{	VI_TM("TestB Other", CNT);
				for (auto n = 0; n < CNT; ++n)
				{	const auto start = vi_tmGetTicks();
					nothing();
					const auto finish = vi_tmGetTicks();
					vi_tmMeasPointAdd(vi_tmMeasPoint(nullptr, "TestB"), finish - start);
				}
			}

			{	VI_TM("TestC First");
				static auto const h = vi_tmMeasPoint(nullptr, "TestC");
				const auto start = vi_tmGetTicks();
				nothing();
				const auto finish = vi_tmGetTicks();
				vi_tmMeasPointAdd(h, finish - start);
			}
			{	VI_TM("TestC Other", CNT);
				for (auto n = 0; n < CNT; ++n)
				{	static auto const h = vi_tmMeasPoint(nullptr, "TestC");
					const auto start = vi_tmGetTicks();
					nothing();
					const auto finish = vi_tmGetTicks();
					vi_tmMeasPointAdd(h, finish - start);
				}
			}

			{	VI_TM("TestD First");
				const auto start = vi_tmGetTicks();
				nothing();
				const auto finish = vi_tmGetTicks();
				static auto const h = vi_tmMeasPoint(nullptr, "TestD");
				vi_tmMeasPointAdd(h, finish - start);
			}
			{	VI_TM("TestD Other", CNT);
				for (auto n = 0; n < CNT; ++n)
				{	const auto start = vi_tmGetTicks();
					nothing();
					const auto finish = vi_tmGetTicks();
					static auto const h = vi_tmMeasPoint(nullptr, "TestD");
					vi_tmMeasPointAdd(h, finish - start);
				}
			}

			{	VI_TM("TestE First");
				VI_TM("TestE");
				nothing();
			}
			{	VI_TM("TestE Other", CNT);
				for (auto n = 0; n < CNT; ++n)
				{	VI_TM("TestE");
					nothing();
				}
			}

			{	VI_TM("TestF First");
				const auto start = vi_tmGetTicks();
				nothing();
				const auto finish = vi_tmGetTicks();
				vi_tmMeasPointAdd(vi_tmMeasPoint(nullptr, "TestF"), finish - start);
			}
			{	VI_TM("TestF Other", CNT);
				const auto h = vi_tmMeasPoint(nullptr, "TestF");
				for (auto n = 0; n < CNT; ++n)
				{	const auto start = vi_tmGetTicks();
					nothing();
					const auto finish = vi_tmGetTicks();
					vi_tmMeasPointAdd(h, finish - start);
				}
			}
		}
		vi_tmThreadAffinityRestore();

		std::cout << "\nTest test - done" << std::endl;
	}
VI_OPTIMIZE_ON

} // namespace

int main()
{	VI_TM_FUNC;
	prn_header();

	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	{
		vi_tmThreadPrepare();

		foo_c();
		test_empty();
		test_instances();
		prn_clock_properties();
		test();

		vi_tmThreadAffinityRestore();
	}

	test_multithreaded();
	test_vi_tmResults();

	std::cout << "\nHello, World!\n" << std::endl;

	assert(0 == errno);
}

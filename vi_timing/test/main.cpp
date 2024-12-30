// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC //-V2573
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include "../vi_timing.h"

#ifdef _WIN32
#	include <Windows.h> // SetThreadAffinityMask
#	include <processthreadsapi.h> // GetCurrentProcessorNumber
#elif defined (__linux__)
#	include <pthread.h> // For pthread_setaffinity_np.
#	include <sched.h> // For sched_getcpu.
#endif

#include <atomic>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::literals;
namespace ch = std::chrono;

namespace
{
	bool cpu_affinity(int core_id = -1)
	{
		if (core_id < 0)
		{
#ifdef _WIN32
			core_id = GetCurrentProcessorNumber();
#elif defined __linux__
			core_id = sched_getcpu();
#else
#	error "Unknown OS."
#endif
		}

		bool result = false;
		if (core_id >= 0)
		{
#ifdef _WIN32
			const auto mask = static_cast<DWORD_PTR>(1U) << core_id; // Set the bit corresponding to the kernel
			result = (0 != SetThreadAffinityMask(GetCurrentThread(), mask));
#elif defined __linux__
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset); // Clearing the bit mask
			CPU_SET(core_id, &cpuset); // Installing the target kernel
			result = (0 == pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset));
#endif
		}

		assert(result);
		return result;
	}

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
//			cpu_affinity();
			return 0;
		}();
}

namespace {
//	VI_OPTIMIZE_OFF
	bool test_multithreaded()
	{	VI_TM_FUNC;
		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOURNAL>, decltype(&vi_tmJournalClose)> h{ vi_tmJournalCreate(), &vi_tmJournalClose };

#ifdef NDEBUG
		static constexpr auto CNT = 2'000'000;
#else
		static constexpr auto CNT = 100'000;
#endif
		static std::atomic<std::size_t> v{};

		std::cout << "\ntest_multithreaded()... " << std::endl;

		auto load = [h = h.get()]
		{	static auto j_load = vi_tmSheet(h, "load");
			vi_tm::meter_t tm{ j_load };

			for (auto n = CNT; n; --n)
			{	const auto s = vi_tmClock();
				const auto f = vi_tmClock();
				const auto name = "check_" + std::to_string(n % 4); //-V112 "Dangerous magic number 4 used"
				vi_tmRecord(vi_tmSheet(h, name.c_str()), f - s, 1);
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
		std::cout << "done" << std::endl;
		return true;
	}
//	VI_OPTIMIZE_ON

	void test_instances()
	{	VI_TM("Additional timers");
		std::cout << "\nAdditional timers...\n";
		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOURNAL>, decltype(&vi_tmJournalClose)> handler{ vi_tmJournalCreate(), &vi_tmJournalClose };
		{	auto h = handler.get();
			{	static auto j1 = vi_tmSheet(h, "long, long, long, very long name");
				vi_tm::meter_t tm1{ j1 };
				{	static auto j2 = vi_tmSheet(h, "100ms * 10");
					vi_tm::meter_t tm2{ j2, 10 };
					for (int n = 0; n < 10; ++n)
					{	std::this_thread::sleep_for(100ms);

						static auto j3 = vi_tmSheet(h, "tm");
						vi_tm::meter_t tm3{ j3 };
						static auto j4 = vi_tmSheet(h, "tm_empty");
						vi_tm::meter_t tm4{ j4 };
					}
				}
			}
			vi_tmReport(h, vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit);
		}
		std::cout << "done" << std::endl;
	}

	void test_empty()
	{	VI_TM_FUNC;
		std::cout << "\ntest_empty()...\n";

		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOURNAL>, decltype(&vi_tmJournalClose)> handler{ vi_tmJournalCreate(), &vi_tmJournalClose };
		{	auto const h = handler.get();

			vi_tmJournalClear(h, "vi_tm");
			vi_tmJournalClear(h, "empty");
			VI_TM_CLEAR("VI_TM");
			VI_TM_CLEAR("EMPTY");

			static auto const jTm = vi_tmSheet(h, "vi_tm");
			static auto const jEmpty = vi_tmSheet(h, "empty");

			for (int n = 0; n < 1'000'000; ++n)
			{
				{
					const auto sTm = vi_tmClock();
					const auto sEmpty = vi_tmClock(); //-V656 Variables 'sTm', 'sEmpty' are initialized through the call to the same function.
					/**/
					const auto fEmpty = vi_tmClock();
					vi_tmRecord(jEmpty, fEmpty - sEmpty, 1);
					const auto fTm = vi_tmClock();
					vi_tmRecord(jTm, fTm - sTm, 1);
				}

				{	VI_TM("VI_TM");
					VI_TM("EMPTY");
				}
			}

			vi_tmTicks_t ticks;
			std::size_t amount;
			std::size_t calls_cnt;
			endl(std::cout);
			vi_tmResult(h, "vi_tm", &ticks, &amount, &calls_cnt);
			std::cout << "vi_tm:\tticks = " << ticks << ",\tamount = " << amount << ",\tcalls = " << calls_cnt << std::endl;
			vi_tmResult(h, "empty", &ticks, &amount, &calls_cnt);
			std::cout << "empty:\tticks = " << ticks << ",\tamount = " << amount << ",\tcalls = " << calls_cnt << std::endl;
			endl(std::cout);
			vi_tmReport(h, vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit | vi_tmShowResolution);
		}

		std::cout << "\ntest_empty() Done\n";
	}
} // namespace

int main()
{	VI_TM_FUNC;
#ifdef NDEBUG
	static constexpr char build_type[] = "Release";
#else
	static constexpr char build_type[] = "Debug";
#endif

	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());
	std::cout <<
#pragma warning(suppress: 4996)
		"Start: " << std::put_time(std::localtime(&tm), "%F %T.\n") <<
		"Build type: " << build_type << "\n" <<
		std::endl;

	std::cout <<
		"Information about the \'vi_timing\' library:" << "\n"
		"\tBuild type: " << reinterpret_cast<const char*>(vi_tmInfo(VI_TM_INFO_BUILDTYPE)) << "\n"
		"\tVer: " << vi_tmInfo(VI_TM_INFO_VER) << "\n"
		"\tBuild number: " << vi_tmInfo(VI_TM_INFO_BUILDNUMBER) << "\n"
		"\tVersion: " << VI_TM_FULLVERSION << "\n" <<
		std::endl;

	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};
	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	std::cout << "Warming... ";
	{	VI_TM("Warming in main()");
		vi_tmWarming(1);
	}
	std::cout << "done" << std::endl;

	assert(0 == errno);

	foo_c();
	test_empty();
	test_instances();
	test_multithreaded();

	endl(std::cout);
	auto results_callback = [](const char* name, vi_tmTicks_t ticks, std::size_t amount, std::size_t calls_cnt, void* data)
		{
			if (0U != calls_cnt)
			{	std::cout << name << ":\tticks = " << ticks << ",\tamount = " << amount << ",\tcalls = " << calls_cnt << std::endl;
			}
			return -1;
		};
	vi_tmResults(nullptr, results_callback, nullptr);
	endl(std::cout);

	std::cout << "\nHello, World!\n" << std::endl;

	assert(0 == errno);
}

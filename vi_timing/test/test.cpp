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
		{	VI_TM("Initialize Common");
			cpu_affinity();
			return 0;
		}();
}

namespace {
//	VI_OPTIMIZE_OFF
	bool test_multithreaded()
	{	VI_TM_FUNC;
		std::unique_ptr<std::remove_pointer_t<VI_TM_HANDLE>, decltype(&vi_tmClose)> h{ vi_tmCreate(), &vi_tmClose };

#ifdef NDEBUG
		static constexpr auto CNT = 2'000'000;
#else
		static constexpr auto CNT = 100'000;
#endif
		static std::atomic<std::size_t> v{};

		std::cout << "\ntest_multithreaded()... " << std::endl;

		auto load = [h = h.get()]
		{	vi_tm::timer_t tm{h, "load"};

			for (auto n = CNT; n; --n)
			{	const auto name = "check_" + std::to_string(n % 4); //-V112 "Dangerous magic number 4 used"
				const auto s = vi_tmClock();
				vi_tmFinish(h, name.c_str(), s);
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
		vi_tmReport(h.get(), vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit);
		std::cout << "done" << std::endl;
		return true;
	}
//	VI_OPTIMIZE_ON

	void test_instances()
	{	VI_TM("Additional timers");
		std::cout << "\nAdditional timers...\n";
		std::unique_ptr<std::remove_pointer_t<VI_TM_HANDLE>, decltype(&vi_tmClose)> handler{ vi_tmCreate(), &vi_tmClose };
		{	auto h = handler.get();
			{	vi_tm::timer_t tm1{ h, "long, long, long, very long name" };
				{	vi_tm::timer_t tm2{ h, "100ms * 10", 10 };
					for (int n = 0; n < 10; ++n)
					{
						std::this_thread::sleep_for(100ms);

						vi_tm::timer_t tm3{ h, "tm" };
						vi_tm::timer_t tm4{ h, "tm_empty" };
					}
				}
			}
			vi_tmReport(h, vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit);
		}
		std::cout << "done" << std::endl;
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
		"\tVer: " << VI_TM_INFO(VI_TM_INFO_VER) << "\n"
		"\tBuild number: " << VI_TM_INFO(VI_TM_INFO_BUILDNUMBER) << "\n"
		"\tVersion: " << reinterpret_cast<const char*>(VI_TM_INFO(VI_TM_INFO_VERSION)) << "\n" <<
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

	foo_c();
	test_instances();
//	git statustest_multithreaded();

	std::cout << "\nHello, World!\n" << std::endl;
}

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include <vi/timing.h>

#include <iomanip>
#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>
#include <atomic>

using namespace std::literals;
namespace ch = std::chrono;

#if defined(_MSC_VER) && defined(_DEBUG)
const auto _dummy0 = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // To automatically call the _CrtDumpMemoryLeaks function when the program ends
const auto _dummy1 = _set_error_mode(_OUT_TO_MSGBOX); // Error sink is a message box. To be able to ignore errors.
#endif

VI_TM_INIT("Timing report:", reinterpret_cast<vi_tmLogSTR_t>(&std::fputs), stdout, 0xE2);
VI_TM("*** Global ***");

namespace {
	void warming(bool all, ch::milliseconds ms)
	{
		if (0 != ms.count())
		{
			std::atomic_bool done = false; // It must be defined before 'threads'!!!
			auto load = [&done] {while (!done) {/**/}}; //-V776

			const auto hwcnt = std::thread::hardware_concurrency();
			std::vector<std::thread> threads((all && hwcnt > 1) ? hwcnt - 1 : 0);
			for (auto& t : threads)
			{
				t = std::thread{ load };
			}

			for (const auto stop = ch::steady_clock::now() + ms; ch::steady_clock::now() < stop;)
			{/*The thread is fully loaded.*/
			}

			done = true;

			for (auto& t : threads)
			{
				t.join();
			}
		}
	}

	bool test()
	{
		std::cout << "\ntest()" << std::endl;

		volatile int dummy = 0;
		{
			VI_TM("SELF_INT");
			dummy = 0;
		}

		constexpr auto CNT = 1'000'000;
		{
			VI_TM("SELF_EXT1", CNT);
			for (int n = 0; n < CNT; ++n)
			{
				VI_TM("SELF_INT1");
				dummy = 0;
			}
		}

		{
			VI_TM("SELF_EXT_2", CNT * 11);
			for (int n = 0; n < CNT; ++n)
			{
				{ VI_TM("SELF_INT_2"); dummy = 0; }

				{ VI_TM("SELF_INT_2"); dummy = 0; } { VI_TM("SELF_INT_2"); dummy = 0; }
				{ VI_TM("SELF_INT_2"); dummy = 0; } { VI_TM("SELF_INT_2"); dummy = 0; }
				{ VI_TM("SELF_INT_2"); dummy = 0; } { VI_TM("SELF_INT_2"); dummy = 0; }
				{ VI_TM("SELF_INT_2"); dummy = 0; } { VI_TM("SELF_INT_2"); dummy = 0; }
				{ VI_TM("SELF_INT_2"); dummy = 0; } { VI_TM("SELF_INT_2"); dummy = 0; }
			}
		}
		std::cout << "Done" << std::endl;
		return true;
	}

	bool test_multithreaded()
	{
		VI_TM_FUNC;
		static constexpr auto CNT = 1'000;
		static std::atomic<std::size_t> v{};

		std::cout << "\ntest_multithreaded()" << std::endl;

		auto load = []()
		{
			VI_TM("load");

			for (auto n = CNT; n; --n)
			{
				VI_TM(("thread_" + std::to_string(n % 10)).c_str());
				v.fetch_add(1, std::memory_order_relaxed);
				std::this_thread::sleep_for(15ms);
			}
		};

		std::vector<std::thread> threads{std::thread::hardware_concurrency() + 8};
		for (auto& t : threads)
		{
			t = std::thread{ load };
		}

		for (auto& t : threads)
		{
			t.join();
		}

		std::cout << "v: " << v << std::endl;
		std::cout << "Done" << std::endl;
		return true;
	}

VI_OPTIMIZE_OFF
	bool test1()
	{
		std::atomic<int> v = 0;

		for(int n = 0; n < 100'000; ++n)
		{
			std::this_thread::yield();

			{
				VI_TM("");
				++v;
			}

			{
				VI_TM("volatile4", 4);
				++v; ++v; ++v; ++v;
			}

			{
				VI_TM("volatile1");
				++v;
			}

			{
				VI_TM("volatile50", 50);
				++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v;
				++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v;
				++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v;
				++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v;
				++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v; ++v;
			}
		}

		return true;
	}
VI_OPTIMIZE_ON

VI_OPTIMIZE_OFF
	bool test_duration()
	{
		std::cout << "\ntest_duration()" << std::endl;

		vi_tmGetTicks();

		{
			static constexpr auto CNT = 1'000'000;
			constexpr char name[] = "B";
			VI_TM(name);
			auto s = vi_tmGetTicks();
			for (int n = 0; n < CNT; n++)
			{
				VI_TM(name);
			}
			auto f = vi_tmGetTicks();
			std::cout << name << ": " << (f - s) / CNT << "\t[" << s << "-" << f << "]\n";
		}
		{
			constexpr char name[] = "A";
			VI_TM(name);
			auto s = vi_tmGetTicks();
			{
				VI_TM(name);
			}
			auto f = vi_tmGetTicks();
			std::cout << name << ": " << (f - s) << "\t[" << s << "-" << f << "]\n";
		}

		std::cout << "Done" << std::endl;
		return true;
	}
VI_OPTIMIZE_ON
}

int main()
{
	const std::time_t t_c = std::chrono::system_clock::to_time_t(ch::system_clock::now());
	std::cout << "Start: " << std::put_time(std::localtime(&t_c), "%F %T.\n") << std::endl;

	warming(true, 500ms);

	foo();

	test1();
	test_duration();
	test();
	test_multithreaded();

	std::cout << "\nHello, World!\n";

	endl(std::cout);
}

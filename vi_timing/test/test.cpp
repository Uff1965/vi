// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC //-V2573
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include <vi/timing.h>

#include <iomanip>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

#if defined(_MSC_VER) && defined(_DEBUG)
const auto _dummy0 = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // To automatically call the _CrtDumpMemoryLeaks function when the program ends
const auto _dummy1 = _set_error_mode(_OUT_TO_MSGBOX); // Error sink is a message box. To be able to ignore errors.
#endif

VI_TM_INIT("Timing report:", reinterpret_cast<vi_tmLogSTR_t>(&std::fputs), stdout, 0xE2);
VI_TM("GLOBAL");

namespace {
	using namespace std::literals;
	namespace ch = std::chrono;

	void warming(bool all, ch::milliseconds ms)
	{
		if (0 != ms.count())
		{
			std::atomic_bool done = false; // It must be defined before 'threads'!!!
			auto load = [&done] {while (!done) {/**/}}; //-V776

			const auto hwcnt = std::thread::hardware_concurrency();
			std::vector<std::thread> threads((all && hwcnt > 1) ? hwcnt - 1 : 0);
			for (auto& t : threads) { t = std::thread{ load }; }
			for (const auto stop = ch::steady_clock::now() + ms; ch::steady_clock::now() < stop;) {/*The thread is fully loaded.*/ }
			done = true;
			for (auto& t : threads) { t.join(); }
		}
	}

	bool test_multithreaded()
	{
		VI_TM_FUNC;

		static constexpr auto CNT = 100'000;
		static std::atomic<std::size_t> v{};

		std::cout << "\nStart: \'test_multithreaded()\'" << std::endl;

		auto load = []
		{	VI_TM("load");

			for (auto n = CNT; n; --n)
			{	VI_TM(("thread_" + std::to_string(n % 5)).c_str());
				v++;
			}
		};

		std::vector<std::thread> threads{std::thread::hardware_concurrency() + 8};
		for (auto& t : threads)
		{	t = std::thread{ load };
		}

		for (auto& t : threads)
		{	t.join();
		}

		std::cout << "v: " << v << std::endl;
		std::cout << "Done" << std::endl;
		return true;
	}
}

int main()
{
	const std::time_t tm = std::chrono::system_clock::to_time_t(ch::system_clock::now());
#pragma warning(suppress: 4996)
	std::cout << "Start: " << std::put_time(std::localtime(&tm), "%F %T.\n") << std::endl;

	warming(true, 500ms);

	foo();
	test_multithreaded();

	std::cout << "\nHello, World!\n";
	endl(std::cout);
}

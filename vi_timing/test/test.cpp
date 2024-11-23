// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC //-V2573
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include "../timing.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
const auto _dummy0 = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // To automatically call the _CrtDumpMemoryLeaks function when the program ends
const auto _dummy1 = _set_error_mode(_OUT_TO_MSGBOX); // Error sink is a message box. To be able to ignore errors.
#endif

VI_TM_INIT(vi_tmShowMask, vi_tmSortBySpeed);
VI_TM("GLOBAL");

namespace {
	using namespace std::literals;
	namespace ch = std::chrono;

	VI_OPTIMIZE_OFF
	bool test_multithreaded()
	{	VI_TM_FUNC;
#ifdef NDEBUG
		static constexpr auto CNT = 2'000'000;
#else
		static constexpr auto CNT = 100'000;
#endif
		static std::atomic<std::size_t> v{};

		std::cout << "\ntest_multithreaded()... " << std::endl;

		auto load = []
		{	VI_TM("load");

			for (auto n = CNT; n; --n)
			{	VI_TM(("check_" + std::to_string(n % 4)).c_str()); //-V112
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

		assert(v.load() == CNT * threads.size());

		std::cout << "v: " << v << "\ndone" << std::endl;
		return true;
	}
}
VI_OPTIMIZE_ON

int main()
{
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
		"\tBuild type: " << static_cast<const char*>(vi_tmInfo(VI_TM_INFO_BUILDTYPE)) << "\n"
		"\tVer: " << reinterpret_cast<std::intptr_t>(VI_TM_INFO()) << "\n"
		"\tBuild number: " << static_cast<unsigned>(reinterpret_cast<std::intptr_t>(VI_TM_INFO(VI_TM_INFO_BUILDNUMBER))) << "\n"
		"\tVersion: " << static_cast<const char*>(VI_TM_INFO(VI_TM_INFO_VERSION)) << "\n"
		"\tCompile time: " << static_cast<const char*>(VI_TM_INFO(VI_TM_INFO_BUILDTIME)) << "\n" <<
		std::endl;

	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};
	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	std::cout << "Warming... ";
	vi_tmWarming();
	std::cout << "done" << std::endl;

	foo_c();
	test_multithreaded();

	std::cout << "\nHello, World!\n" << std::endl;
}

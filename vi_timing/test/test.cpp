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
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
const auto _dummy0 = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // To automatically call the _CrtDumpMemoryLeaks function when the program ends
const auto _dummy1 = _set_error_mode(_OUT_TO_MSGBOX); // Error sink is a message box. To be able to ignore errors.
#endif

VI_TM_INIT("Timing report:\n", reinterpret_cast<vi_tmLogSTR_t>(&std::fputs), stdout, vi_tmSortBySpeed | vi_tmShowMask);
VI_TM("GLOBAL");

namespace {
	using namespace std::literals;
	namespace ch = std::chrono;

	VI_OPTIMIZE_OFF
	bool test_multithreaded()
	{
		VI_TM_FUNC;

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
			{	VI_TM(("check_" + std::to_string(n % 4)).c_str());
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

		std::cout << "v: " << v << std::endl;
		std::cout << "done" << std::endl;
		return true;
	}
}
VI_OPTIMIZE_ON

VI_OPTIMIZE_OFF
bool test_quant()
{
	std::cout << "\ntest_quant()... " << std::endl;

	{
		const auto s = vi_tmGetTicks();
		const auto f = vi_tmGetTicks();
		std::cout << "s: " << s << "\n";
		std::cout << "f: " << f << "\n";
		std::cout << "f - s: " << f - s << "\n";
		std::cout << "sizeof(vi_tmTicks_t): " << sizeof(vi_tmTicks_t) << " byte" << "\n";
		endl(std::cout);
	}

	const auto [step, dur] = []
	{	constexpr auto CNT = 100'000;
		std::this_thread::yield();
		const auto s = ch::high_resolution_clock::now();
		auto last = vi_tmGetTicks();
		const auto first = last;
		for (int n = 0; n < CNT; ++n)
			last = vi_tmGetTicks();
		const auto f = ch::high_resolution_clock::now();
		return std::tuple{ (last - first) / CNT, (ch::nanoseconds(f - s).count() * 1'000) / (last - first) };
	}();

	std::atomic_bool done = false; // It must be defined before 'threads'!!!
	std::vector<std::thread> threads(std::thread::hardware_concurrency());
	for (auto& t : threads) { t = std::thread{ [&done] {while (!done) {/**/}}}; }

	const auto mes = [step]
	{	std::this_thread::yield();
		const auto first = vi_tmGetTicks();
		auto last = first;
		for (auto current = vi_tmGetTicks(); current - last < 1'000 * step; current = vi_tmGetTicks())
		{	last = current;
		}
		return last - first;
	}();

	done = true;
	for (auto& t : threads) { t.join(); }

	std::cout << "Quantum of time: " << mes << " ticks * " << dur << " ps = " << mes * dur / 1'000'000 << " us.\n";
	std::cout << "Step: " << step << " ticks * " << dur << " ps = " << step * dur << " ps.\n";
	std::cout << "done" << std::endl;
	return true;
}
VI_OPTIMIZE_ON

int main()
{
	const std::time_t tm = std::chrono::system_clock::to_time_t(ch::system_clock::now());
#pragma warning(suppress: 4996)
	std::cout << "Start: " << std::put_time(std::localtime(&tm), "%F %T.\n");
	endl(std::cout);

	{
		std::cout << "Build type: " << static_cast<const char*>(vi_tmInfo(VI_TM_BUILDTYPE)) << "\n";
		std::cout << "Ver: " << reinterpret_cast<std::ptrdiff_t>(VI_TM_INFO()) << "\n";
		std::cout << "Version: " << static_cast<const char*>(VI_TM_INFO(VI_TM_INFO_VERSION)) << "\n";
		std::cout << "Compile time: " << static_cast<const char*>(VI_TM_INFO(VI_TM_INFO_TIME)) << "\n";
		endl(std::cout);
	}

	struct space_out final: std::numpunct<char> {
		char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};
	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	std::cout << "Warming... ";
	vi_tmWarming();
	std::cout << "done";
	endl(std::cout);

	foo_c();
	test_multithreaded();
	test_quant();

	std::cout << "\nHello, World!\n";
	endl(std::cout);
}

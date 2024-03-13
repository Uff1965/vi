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

		std::cout << "\ntest_multithreaded()... " << std::endl;

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
		std::cout << "done" << std::endl;
		return true;
	}
}

bool test_quant()
{
	std::cout << "\ntest_quant()... " << std::endl;

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

int main()
{
	struct space_out : std::numpunct<char> {
		char do_thousands_sep() const override { return '\''; }  // separate with spaces
		std::string do_grouping() const override { return "\3"; } // groups of 1 digit
	};
	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	const std::time_t tm = std::chrono::system_clock::to_time_t(ch::system_clock::now());
#pragma warning(suppress: 4996)
	std::cout << "Start: " << std::put_time(std::localtime(&tm), "%F %T.\n");

	std::cout << "Build type: ";
#ifdef NDEBUG
	std::cout << "Release";
#else
	std::cout << "Debug";
#endif
	endl(std::cout);

	std::cout << "\nWarming... ";
	warming(true, 500ms);
	std::cout << "done";
	endl(std::cout);

	foo();
	test_multithreaded();
	test_quant();

	std::cout << "\nHello, World!\n";
	endl(std::cout);
}

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC //-V2573
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include "../vi_timing.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <vector>

#if false
#	undef VI_OPTIMIZE_ON
#	define VI_OPTIMIZE_ON
#	undef VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_OFF
#endif

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
			vi_tmCurrentThreadAffinityFixate();
			vi_tmWarming(1, 500);
			vi_tmThreadYield();
			return 0;
		}();
}

namespace
{
	void report_RAW(VI_TM_HJOUR h)
	{
		struct data_t
		{	std::size_t name_max_len_ = 0;
		} data;

		auto cb_name = [](VI_TM_HMEAS m, void *data)
			{	auto d = static_cast<data_t *>(data);
				const char *name = nullptr;
				vi_tmMeasuringGet(m, &name, nullptr);
				if (name && *name && d->name_max_len_ < std::strlen(name))
				{	d->name_max_len_ = std::strlen(name);
				}
				return 0;
			};
		vi_tmMeasuringEnumerate(h, cb_name, &data);

		auto results_callback = [](VI_TM_HMEAS m, void* data)
			{	auto d = static_cast<data_t *>(data);
				const char *name;
				vi_tmMeasuringRAW_t meas;
				vi_tmMeasuringGet(m, &name, &meas);
				std::cout << std::left << std::setw(d->name_max_len_) << name << ":" <<
				std::right << std::scientific <<
#ifdef VI_TM_STAT_USE_WELFORD
				" mean = " << meas.flt_mean_ << ","
				" ss = " << meas.flt_ss_ << ","
				" cnt = " << std::setw(12) << meas.flt_cnt_ << ","
#endif
				" sum = " << std::setw(15) << meas.sum_ << ","
				" amt = " << std::setw(12) << meas.amt_ << ","
				" calls = " << std::setw(12) << meas.calls_ <<
				std::endl;
				return 0;
			};
		vi_tmMeasuringEnumerate(h, results_callback, &data);
	}

VI_OPTIMIZE_OFF
	void test_multithreaded()
	{	VI_TM("test_multithreaded");
#ifdef NDEBUG
		static constexpr auto CNT = 2'000'000;
#else
		static constexpr auto CNT = 100'000;
#endif

		std::cout << "\nTest multithreaded():" << std::endl;

		static std::atomic<std::size_t> v{};
		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> h{ vi_tmJournalCreate(), &vi_tmJournalClose };
		auto thread_fn = [h = h.get()]
		{	static auto j_load = vi_tmMeasuring(h, "thread_fn");
			vi_tm::measurer_t tm{ j_load };

			for (auto n = CNT; n; --n)
			{	const auto s = vi_tmGetTicks();
				const auto f = vi_tmGetTicks();
				const auto name = "check_" + std::to_string(n % 4); //-V112 "Dangerous magic number 4 used"
				vi_tmMeasuringRepl(vi_tmMeasuring(h, name.c_str()), f - s, 1);
				v++;
			}
		};

		std::vector<std::thread> threads{ 2 * std::thread::hardware_concurrency() };
		for (auto& t : threads)
		{	t = std::thread{ thread_fn };
		}

		for (auto& t : threads)
		{	t.join();
		}

		std::cout << "v (" << v << ") must be equal to " << CNT << "*" << threads.size() << " (" << CNT * threads.size() << ")";
		if (v.load() != CNT * threads.size())
		{	std::cerr << " - FAIL!!!\n";
			assert(false);
		}
		else
		{	std::cout << " - OK\n";
		}

		std::cout << "Timing Test multithreaded:\n";
		std::cout << "RAW:\n";
		report_RAW(h.get());
		std::cout << "Report:\n";
		vi_tmReport(h.get(), vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit | vi_tmShowResolution);
		std::cout << "Test multithreaded - done" << std::endl;
	}
VI_OPTIMIZE_ON

	void test_sleep()
	{	VI_TM("Test sleep");
		std::cout << "\nTest sleep:\n";
		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> handler{ vi_tmJournalCreate(), &vi_tmJournalClose };
		{	auto h = handler.get();
			{	auto m = vi_tmMeasuring(h, "100ms");
				vi_tm::measurer_t tm{ vi_tmMeasuring(h, "100ms*10"), 10U };
				for (int n = 0; n < 10; ++n)
				{	vi_tm::measurer_t tm2{ m };
					std::this_thread::sleep_for(100ms);
				}
			}
			vi_tmReport(h, vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit);
		}
		std::cout << "Test sleep - done" << std::endl;
	}

VI_OPTIMIZE_OFF
	void test_empty()
	{	VI_TM("test_empty");
		std::cout << "\nTest test_empty:\n";

		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> handler{ vi_tmJournalCreate(), &vi_tmJournalClose };
		{	auto const h = handler.get();

			vi_tmJournalReset(h, "vi_tm");
			vi_tmJournalReset(h, "empty");

			static auto const jTm = vi_tmMeasuring(h, "vi_tm");
			static auto const jEmpty = vi_tmMeasuring(h, "empty");

			for (int n = 0; n < 1'000'000; ++n)
			{
				{
					const auto sTm = vi_tmGetTicks();
					const auto sEmpty = vi_tmGetTicks(); //-V656 Variables 'sTm', 'sEmpty' are initialized through the call to the same function.
					/**/
					const auto fEmpty = vi_tmGetTicks();
					vi_tmMeasuringRepl(jEmpty, fEmpty - sEmpty, 1);
					const auto fTm = vi_tmGetTicks();
					vi_tmMeasuringRepl(jTm, fTm - sTm, 1);
				}
			}

			const char *name = nullptr;
			vi_tmMeasuringRAW_t data;
			vi_tmMeasuringGet(vi_tmMeasuring(h, "vi_tm"), &name, &data);
#ifdef VI_TM_STAT_USE_WELFORD
			std::cout << "vi_tm:\tmean = " << std::setprecision(3) << data.flt_mean_ << ",\tamount = " << data.amt_ << ",\tcalls = " << data.calls_ << std::endl;
			vi_tmMeasuringGet(vi_tmMeasuring(h, "empty"), &name, &data);
			std::cout << "empty:\tmean = " << std::setprecision(3)  << data.flt_mean_ << ",\tamount = " << data.amt_ << ",\tcalls = " << data.calls_ << std::endl;
#else
			std::cout << "vi_tm:\tticks = " << std::setw(16) << data.sum_ << ",\tamount = " << data.amt_ << ",\tcalls = " << data.calls_ << std::endl;
			vi_tmMeasuringGet(vi_tmMeasuring(h, "empty"), &name, &data);
			std::cout << "empty:\tticks = " << std::setw(16)  << data.sum_ << ",\tamount = " << data.amt_ << ",\tcalls = " << data.calls_ << std::endl;
#endif
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
			"\tBuild type: " << static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_BUILDTYPE)) << "\n"
			"\tVer: " << *static_cast<const unsigned*>(vi_tmStaticInfo(VI_TM_INFO_VER)) << "\n"
			"\tBuild number: " << *static_cast<const unsigned*>(vi_tmStaticInfo(VI_TM_INFO_BUILDNUMBER)) << "\n"
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
		if (auto ptr = static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_RESOLUTION)))
		{	std::cout << "\nResolution: " << std::setprecision(3) << *ptr << " ticks";
		}
		if (auto ptr = static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_DURATION)))
		{	std::cout << "\nDuration: " << std::setprecision(3) << 1e9 * *ptr << " ns.";
		}
		if (auto ptr = static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_OVERHEAD)))
		{	std::cout << "\nAdditive: " << std::setprecision(3) << *ptr << " ticks";
		}
		if (auto ptr = static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_UNIT)))
		{	std::cout << "\nTick: " << std::setprecision(3) << 1e9 * *ptr << " ns.";
		}
		std::cout << "\nClock properties - done" << std::endl;
	}

	void test_report()
	{	VI_TM("test_report");
		std::cout << "\nTest vi_tmReport:" << std::endl;

//#	define TINY_SET_SAMPLES
#	ifdef TINY_SET_SAMPLES
		static constexpr VI_TM_TDIFF samples_simple[] = { 34, 81, 17 }; // 132/3=44; 2198/2=1099
		static constexpr VI_TM_TDIFF samples_multiple[] = { 34, }; // 34/1=34; 1156/0=...
		static constexpr auto M = 2;
#	else
		static constexpr VI_TM_TDIFF samples_simple[] =
		{	40, 60, 37, 16, 44, 37, 22, 48, 37, 37, 48, 40, 39, 37, 52, 33, 44, 55, 42, 19,
			36, 33, 43, 26, 41, 34, 40, 13, 27, 35, 21, 41, 35, 40, 41, 54, 37, 29, 35, 56,
			35, 39, 26, 49, 25, 37, 30, 40, 32, 44, 48, 33, 56, 23, 47, 37, 34, 40, 58, 64,
			35, 53, 48, 36, 70, 38, 41, 41, 45, 34, 32, 57, 39, 19, 43, 50, 23, 46, 45, 53,
			42, 45, 44, 29, 54, 61, 32, 30, 39, 32, 47, 32, 42, 56, 38, 33, 36, 38, 35, 42,
			28, 53, 50, 48, 32, 25, 34, 45, 32, 39, 34, 42, 43, 50, 52, 42, 41, 41, 48, 38,
			42, 25, 39, 43, 65, 34, 46, 61, 55, 39, 41, 42, 37, 51, 38, 46, 31, 38, 48, 54,
			48, 50, 28, 43, 48, 54, 38, 48, 18, 37, 47, 43, 38, 48, 36, 56, 32, 58, 46, 58,
			39, 41, 31, 34, 33, 47, 43, 34, 39, 55, 39, 27, 39, 54, 24, 38, 44, 20, 40, 50,
			43, 39, 33, 41, 46, 37, 42, 51, 57, 29, 52, 36, 57, 56, 42, 39, 35, 38, 31, 42,
		}; // 8139/200=40.695; 19798.39442/199=99.48941920
		static constexpr VI_TM_TDIFF samples_multiple[] =
		{	35, 39, 43, 40, 38, 43, 41, 33, 39, 37,
		}; // 388/10=38.8; 93.60/9=10.40
		static constexpr auto M = 10; // (8139+10*388)/(200+10*10)=12019/300=40,063(3); (19798,39442+10*93,60)=20734,39442
#	endif

		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> journal{ vi_tmJournalCreate(), vi_tmJournalClose };
		{	const auto unit = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_UNIT));
			const auto m = vi_tmMeasuring(journal.get(), "Sample");
			for (auto x : samples_simple)
			{	vi_tmMeasuringRepl(m, static_cast<VI_TM_TDIFF>(x / unit));
			}
			for (auto x : samples_multiple)
			{	vi_tmMeasuringRepl(m, M * static_cast<VI_TM_TDIFF>(x / unit), M);
			}
		}
		vi_tmReport(journal.get(), vi_tmShowDuration | vi_tmShowOverhead | vi_tmShowUnit | vi_tmShowResolution);
		std::cout << "RAW:\n";
		report_RAW(journal.get());

		std::cout << "Test vi_tmReport - done" << std::endl;
	}

	void test_sleep2()
	{	VI_TM("test_sleep2");
		std::cout << "\nTest test_sleep2:\n";

		{	std::mt19937 gen(std::random_device{}());
			std::normal_distribution dist(0.1, 0.01);
			for (int i = 0; i < 100; ++i)
			{	double val;
				do
				{	val = dist(gen);
				} while (val < 0.015 || val > 1.5);

				{	VI_TM("SLEEP_FOR");
					std::this_thread::sleep_for(std::chrono::duration<double>(val));
				}
			}
		}
		std::cout << "Test test_sleep2 - done" << std::endl;
	}

VI_OPTIMIZE_OFF
	void test_access()
	{	VI_TM("Test test_access");
		std::cout << "\nTest test_access:\n";

		static int v1 = 0;
		static volatile int v2 = 0;
		static std::atomic<int> v3 = 0;
		thread_local int v4 = 0;
		static std::shared_mutex smtx;
		static std::mutex mtx;
		static std::recursive_mutex rmtx;
		static std::timed_mutex tmtx;

		auto a0 = [] { VI_TM("*empty"); };
		auto a1 = [] { VI_TM("*normal"); v1 = 777; };
		auto a2 = [] { VI_TM("*volatile"); v2 = 777; };
		auto a3 = [] { VI_TM("*atomic_relax"); v3.store(777, std::memory_order_relaxed); };
		auto a4 = [] { VI_TM("*thread_local"); v4 = 777; };
		auto a5 = [] { VI_TM("*atomic"); v3 = 777; };
		auto a6 = [] { VI_TM("*shared_mutex"); std::lock_guard lg{ smtx  }; v1 = 777; };
		auto a7 = [] { VI_TM("*mutex"); std::lock_guard lg{ mtx }; v1 = 777; };
		auto a8 = [] { VI_TM("*recursive_m"); std::lock_guard lg{ rmtx }; v1 = 777; };
		auto a9 = [] { VI_TM("*timed_mutex"); std::lock_guard lg{ tmtx }; v1 = 777; };

		void(*arr[])() = { a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, };
		std::sort(std::begin(arr), std::end(arr));
		for (unsigned n = 1; n; --n)
		{	do
			{	for (auto &&f : arr)
				{	std::this_thread::yield();
					f();
				}
			} while (std::next_permutation(std::begin(arr), std::end(arr)));
		}

		std::cout << "Test test_access - done" << std::endl;
	}
VI_OPTIMIZE_ON
} // namespace

int main()
{	VI_TM_FUNC;
	prn_header();

	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	vi_tmWarming(1, 500);
	vi_tmThreadYield();

	foo_c();

	test_empty();
	test_sleep();
	test_sleep2();
	prn_clock_properties();

	test_report();
	test_multithreaded();
	test_access();

	report_RAW(nullptr);

	vi_tmCurrentThreadAffinityRestore();

	std::cout << "\nHello, World!\n" << std::endl;

	assert(0 == errno);
}

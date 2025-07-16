// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include "vi_timing/vi_timing_proxy.h"
#ifndef VI_TM_SHARED
#	include "../source/build_number_maker.h"
#endif

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

	VI_TM_INIT(vi_tmSortBySpeed, "Global timing report:\n", vi_tmShowMask);
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
			vi_CurrentThreadAffinityFixate();
			vi_Warming();
			return 0;
		}();

	void report_RAW(VI_TM_HJOUR h)
	{
		struct data_t
		{	std::size_t name_max_len_ = 0;
		} data;

		auto cb_name = [](VI_TM_HMEAS m, void *data)
			{	auto d = static_cast<data_t *>(data);
				const char *name = nullptr;
				vi_tmMeasurementGet(m, &name, nullptr);
				if (name && d->name_max_len_ < std::strlen(name))
				{	d->name_max_len_ = std::strlen(name);
				}
				return 0;
			};
		vi_tmMeasurementEnumerate(h, cb_name, &data);

		auto results_callback = [](VI_TM_HMEAS m, void* data)
			{	auto d = static_cast<data_t *>(data);
				const char *name;
				vi_tmMeasurementStats_t meas;
				vi_tmMeasurementGet(m, &name, &meas);
				std::cout << std::left << std::setw(d->name_max_len_) << name << ":" <<
				std::right << std::scientific <<
				" calls = " << std::setw(8) << meas.calls_ << "," <<
#if VI_TM_STAT_USE_BASE
				" amt = " << std::setw(8) << meas.amt_ << ","
				" sum = " << std::setw(15) << meas.sum_ << "," <<
#endif
#if VI_TM_STAT_USE_FILTER
				std::setprecision(3) << std::defaultfloat << 
				" flt_calls = " << std::setw(8) << meas.flt_calls_ << ","
				" flt_amt = " << std::setw(8) << meas.flt_amt_ << ","
				" flt_mean = " << std::setw(8) << meas.flt_mean_ << ","
				" flt_ss = " << std::setw(8) << meas.flt_ss_ <<
#endif
#if VI_TM_STAT_USE_MINMAX
				" min = " << std::setw(9) << meas.min_ << ","
				" max = " << std::setw(9) << meas.max_ << "," <<
#endif
				std::endl;
				return 0;
			};
		vi_tmMeasurementEnumerate(h, results_callback, &data);
	}

	inline std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> create_journal()
	{	return { vi_tmJournalCreate(), &vi_tmJournalClose };
	}

	void test_multithreaded()
	{	VI_TM("test_multithreaded");
#ifdef NDEBUG
		static constexpr auto CNT = 2'000'000;
#else
		static constexpr auto CNT = 100'000;
#endif

		std::cout << "\nTest multithreaded():" << std::endl;

		static std::atomic<std::size_t> v{};
		auto h = create_journal();
		auto thread_fn = [h = h.get()]
		{	static auto j_load = vi_tmMeasurement(h, "thread_fn");
			vi_tm::measurer_t tm{ j_load };

			for (auto n = CNT; n; --n)
			{	const auto s = vi_tmGetTicks();
				const auto f = vi_tmGetTicks();
				const auto name = "check_" + std::to_string(n % 4);
				vi_tmMeasurementAdd(vi_tmMeasurement(h, name.c_str()), f - s, 1);
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
		vi_tmReport(h.get(), vi_tmShowMask);
		std::cout << "Test multithreaded - done" << std::endl;
	}

	void test_sleep()
	{	VI_TM("Test sleep");
		std::cout << "\nTest sleep:\n";
		auto journal = create_journal();
		if(	auto j = journal.get())
		{	auto m = vi_tmMeasurement(j, "100ms");
			{	vi_tm::measurer_t tm{ vi_tmMeasurement(j, "100ms*10"), 10U };
				for (int n = 0; n < 10; ++n)
				{	vi_tm::measurer_t tm2{ m };
					std::this_thread::sleep_for(100ms);
				}
			}
			vi_tmReport(j, vi_tmShowDuration | vi_tmShowDurationEx | vi_tmShowOverhead | vi_tmShowUnit);
		}
		std::cout << "Test sleep - done" << std::endl;
	}

	void test_empty()
	{	VI_TM("test_empty");
		std::cout << "\nTest test_empty:\n";

		auto journal = create_journal();
		{
			auto const j = journal.get();
			auto const jTm = vi_tmMeasurement(j, "vi_tm");
			auto const jEmpty = vi_tmMeasurement(j, "empty");

			for (int n = 0; n < 1'000'000; ++n)
			{
				const auto sTm = vi_tmGetTicks();
				const auto sEmpty = vi_tmGetTicks();
				/**/
				const auto fEmpty = vi_tmGetTicks();
				vi_tmMeasurementAdd(jEmpty, fEmpty - sEmpty, 1);
				const auto fTm = vi_tmGetTicks();
				vi_tmMeasurementAdd(jTm, fTm - sTm, 1);
			}

			vi_tmReport(j, vi_tmShowMask);
		}

		std::cout << "Test test_empty - Done" << std::endl;
	}

	void prn_header()
	{	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());

#pragma warning(suppress: 4996) // Suppress MSVC warning: 'localtime': This function or variable may be unsafe.
		std::cout << "\nStart: " << std::put_time(std::localtime(&tm), "%F %T.\n") <<
			std::endl;

		std::cout <<
			"Information about the \'vi_timing\' library:\n"
			"\tVersion: " << VI_TM_FULLVERSION << "\n"
			"\tBuild type: " << static_cast<const char *>(vi_tmStaticInfo(VI_TM_INFO_BUILDTYPE)) << "\n"
			"\tVer: " << *static_cast<const unsigned *>(vi_tmStaticInfo(VI_TM_INFO_VER)) << "\n"
			"\tBuild number: " << *static_cast<const unsigned *>(vi_tmStaticInfo(VI_TM_INFO_BUILDNUMBER)) << "\n";
	}

	void test_warming()
	{	std::cout << "\nTest test_warming:\n";
	
		std::this_thread::sleep_for(1s);

		{	VI_TM("Warming(0)");
			vi_Warming(0, 20000);
		}

		std::this_thread::sleep_for(1s);

		{	VI_TM("Warming(4)");
			vi_Warming(4, 10000);
		}

		std::this_thread::sleep_for(1s);

		{	VI_TM("Warming(1)");
			vi_Warming(1, 10000);
		}

		std::this_thread::sleep_for(1s);

		assert(0 == errno);
		std::cout << "Test test_warming - done" << std::endl;
	}

	void prn_clock_properties()
	{	std::cout << "\nClock properties:";
		if (auto ptr = static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_RESOLUTION)))
		{	std::cout << "\nResolution: " << std::setprecision(3) << *ptr << " ticks";
		}
		if (auto ptr = static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_DURATION)))
		{	std::cout << "\nDuration: " << std::setprecision(3) << 1e9 * *ptr << " ns.";
		}
		if (auto ptr = static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_DURATION_EX)))
		{	std::cout << "\nDuration ex: " << std::setprecision(3) << 1e9 * *ptr << " ns.";
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

		auto journal = create_journal();
		{	const auto unit = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_UNIT));
			const auto m = vi_tmMeasurement(journal.get(), "Sample");
			for (auto x : samples_simple)
			{	vi_tmMeasurementAdd(m, static_cast<VI_TM_TDIFF>(x / unit));
			}
			for (auto x : samples_multiple)
			{	vi_tmMeasurementAdd(m, M * static_cast<VI_TM_TDIFF>(x / unit), M);
			}
		}
		std::cout << "RAW:\n";
		report_RAW(journal.get());
		vi_tmReport(journal.get(), vi_tmShowMask);

		std::cout << "Test vi_tmReport - done" << std::endl;
	}

	void normal_distribution()
	{	VI_TM("normal_distribution");
		std::cout << "\nTest normal_distribution:\n";

		constexpr auto MULT = 1'000;
		constexpr auto CNT = 1'000'000;
		constexpr auto MEAN = 1e6;
		constexpr auto CV = 0.05;
		std::cout << "This test generates normally distributed numbers "
			"with mean = " << MEAN << " ticks and CV = " << CV << ".\n"
			"There are " << CNT + MULT * CNT / MULT << " numbers in total, " << CNT <<
			" are added one by one and " << CNT / MULT <<  " by " << MULT << ".\n";

		const auto j = create_journal();
		const auto m = vi_tmMeasurement(j.get(), "ITEM");
		
		std::mt19937 gen{ /*std::random_device{}()*/ };
		std::normal_distribution dist(MEAN, CV * MEAN);

		for (int i = 0; i < CNT; ++i)
		{	const auto v = dist(gen);
			assert(v >= 0);
			vi_tmMeasurementAdd(m, static_cast<VI_TM_TICK>(std::round(v)), 1);
		}

		std::normal_distribution distM(MEAN * MULT, CV * MEAN * MULT);
		for (int i = 0; i < CNT / MULT; ++i)
		{	const auto v = distM(gen);
			assert(v >= 0);
			vi_tmMeasurementAdd(m, static_cast<VI_TM_TICK>(std::round(v)), MULT);
		}

		std::cout << "\nRAW report:\n";
		report_RAW(j.get());
		std::cout << "\nReport:\n";
		vi_tmReport(j.get(), vi_tmShowMask);

		vi_tmMeasurementStats_t raw;
		vi_tmMeasurementGet(m, nullptr, &raw);
#if VI_TM_STAT_USE_BASE
		assert(raw.amt_ == CNT + CNT);
#endif
		assert(raw.calls_ == CNT + CNT / MULT);
#if VI_TM_STAT_USE_FILTER
		assert(std::abs(raw.flt_mean_ - MEAN) / MEAN < 0.01);
		assert(std::abs(std::sqrt(raw.flt_ss_ / raw.flt_amt_) / MEAN - CV) < 0.01);
#endif
		std::cout << "Test normal_distribution - done" << std::endl;
	}

	void test_access()
	{	VI_TM("Test test_access");
		std::cout << "\nTest test_access:\n";

		static int v_normal = 0;
		static volatile int v_volatile = 0;
		static std::atomic<int> v_atomic = 0;
		static thread_local int v_thread_local = 0;
		static std::mutex mtx;
		static std::recursive_mutex mtx_r;
		static std::timed_mutex mtx_t;
		static std::shared_mutex mtx_s;

#define VI_TM_EX(j, name) static const auto m = vi_tmMeasurement(j, name); vi_tm::measurer_t _{m}
		static void(*arr[])(VI_TM_HJOUR) = {
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*empty"); },
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*normal"); ++v_normal; },
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*volatile"); v_volatile = v_volatile + 1; },
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*thread_local"); ++v_thread_local; },
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*atomic"); ++v_atomic; },
//			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*atomic_relax"); v_atomic.fetch_add(1, std::memory_order_relaxed); },
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*mutex"); std::lock_guard lg{ mtx }; ++v_normal; },
//			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*mutex_shared"); std::lock_guard lg{ mtx_s }; ++v_normal; },
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*mutex_recurs"); std::lock_guard lg{ mtx_r }; ++v_normal; },
			[] (VI_TM_HJOUR h ){ VI_TM_EX(h, "*mutex_timed"); std::lock_guard lg{ mtx_t }; ++v_normal; },
			[] (VI_TM_HJOUR h ){ static const auto m = vi_tmMeasurement(h, "*VI_TM"); vi_tm::measurer_t _{ m }; ++v_normal; },
			[] (VI_TM_HJOUR h ){ vi_tm::measurer_t _{ vi_tmMeasurement(h, "*VI_XX") }; ++v_normal; },
		};
#undef VI_TM_EX

		std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> journal{ vi_tmJournalCreate(), vi_tmJournalClose };
		std::sort(std::begin(arr), std::end(arr));

		constexpr static auto CNT = []
			{	std::size_t result = 1;
				std::size_t n = std::size(arr);
				for (std::size_t i = 2; i <= n; ++i)
				{	result *= i;
				}
				return result;
			}();

		endl(std::cout);
		std::size_t n = 0;
		do
		{	for (auto f : arr)
			{	std::this_thread::yield();
				f(journal.get());
			}
			if (0 == ++n % 10'000)
			{	std::cout << "\r" << std::setw(9) << n << " / " << CNT << std::flush;
			}
		} while (std::next_permutation(std::begin(arr), std::end(arr)));
		endl(std::cout);

		std::cout << "RAW report:\n";
		report_RAW(journal.get());
		std::cout << "Report:\n";
		unsigned flags = 0U;
		flags |= vi_tmShowMask;
		flags |= vi_tmSortBySpeed;
//		flags |= vi_tmDoNotSubtractOverhead;
		vi_tmReport(journal.get(), flags);

		std::cout << "Test test_access - done" << std::endl;
	}

	void test_misc()
	{
		{	VI_TM("test_misc:");
			std::cout << "\nTest test_misc:\n";

			static VI_TM("test_misc: Local Static");

			if (VI_TM("test_misc: In IF"); false)
				VI_TM("test_misc: Local If");
			else
				VI_TM("test_misc: Local Else");

			std::this_thread::sleep_for(1s);
		}

		{	vi_tm::measurer_t m{vi_tmMeasurement(VI_TM_HGLOBAL, "test_misc: xxx")};
			std::this_thread::sleep_for(1s);
			m.finish();

			m.start(100);
			std::this_thread::sleep_for(1ms);
			m.stop();

			m.start(3);
			std::this_thread::sleep_for(3s);
			m.finish();

			m.start(1);
			std::this_thread::sleep_for(1s);
		}

		std::cout << "Test test_misc - done" << std::endl;
	}

	void busy()
	{	volatile auto f = 0.0;
		for (auto n = 10'000U; n; --n)
		{	// To keep the CPU busy.
			f = (f + std::sin(n) * std::cos(n)) / 1.0001;
			std::atomic_thread_fence(std::memory_order_relaxed);
		}
	};

	void test_busy()
	{	VI_TM("Test test_busy");
		std::cout << "\nTest test_busy:\n";

		for (int n = 0; n < 1'000; ++n)
		{	VI_TM("busy");
			busy();
		}

		std::cout << "Test test_busy - done" << std::endl;
	}
} // namespace

int main()
{	VI_TM_FUNC;
	prn_header();

	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	vi_ThreadYield();

	prn_clock_properties();
	//foo_c();

	//test_busy();
	//// test_warming();
	//test_misc();
	test_empty();
	//test_sleep();
	normal_distribution();

	//test_report();
	//test_multithreaded();
	test_access();
	//std::cout << "\nRAW report:\n";
	//report_RAW(VI_TM_HGLOBAL);

	vi_CurrentThreadAffinityRestore();

	vi_tmInit();
	vi_tmFinit();

	std::cout << "\nHello, World!\n" << std::endl;

	assert(0 == errno);
}

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "../vi_timing.h"

#ifdef _WIN32
#	include <Windows.h> // SetThreadAffinityMask
#	include <processthreadsapi.h> // GetCurrentProcessorNumber
#elif defined (__linux__)
#	include <pthread.h> // For pthread_setaffinity_np.
#	include <sched.h> // For sched_getcpu.
#else
#	error "Unknown OS."
#endif

#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

using namespace std::literals;
namespace ch = std::chrono;

namespace
{
	bool cpu_setaffinity(int core_id = -1)
	{
		if (core_id < 0)
		{
#ifdef _WIN32
			core_id = GetCurrentProcessorNumber();
#elif defined __linux__
			core_id = sched_getcpu();
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
			result = (0 == pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset));
#endif
		}

		assert(result);
		return result;
	}

	// Initialize the vi_timing library and order the report.
	VI_TM_INIT(vi_tmSortBySpeed, "Global timing report:\n", vi_tmShowDuration, vi_tmShowOverhead, vi_tmShowUnit, vi_tmShowResolution);
	// Checking measurements taking into account initialization and deletion of static objects.
	VI_TM("GLOBAL");

	const auto _ =
		(
#if defined(_MSC_VER) && defined(_DEBUG)
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF), // To automatically call the _CrtDumpMemoryLeaks function when the program ends
			_set_error_mode(_OUT_TO_MSGBOX), // Error sink is a message box. To be able to ignore errors.
#endif
			cpu_setaffinity(), // Sets a processor affinity mask for the specified thread.
			0
		);
}

int main_()
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

	std::cout << "\nHello, World!\n" << std::endl;

	assert(0 == errno);

	return 0;
}

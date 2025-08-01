// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*****************************************************************************\
* This file is part of the 'vi_timing' library.
* 
* 'vi_timing' is a compact and lightweight library for measuring code execution
* time in C and C++. 
*
* This library was created for experimental and educational use. Please keep 
* expectations reasonable. If you find bugs or have suggestions for 
* improvement, contact <programmer.amateur@proton.me>.
*
* No warranties � see LICENSE.txt in project root.
* Business Source License 1.1 (BSL-1.1):
*   � Permitted for non-commercial use only.
*   � Change Date: 2029-09-01 � thereafter under GNU GPLv3.
* 
* Attribution notice must be preserved in all copies and derivatives:
*    �vi_timing Library � A.Prograamar�
* 
* For commercial licensing inquiries: <programmer.amateur@proton.me>
\*****************************************************************************/

#include "../vi_timing_c.h"
#include "version.h"

#if VI_TM_USE_STDCLOCK
	// Use standard clock
#	include <time.h> // for timespec_get
	VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) noexcept
	{	struct timespec ts;
		(void)timespec_get(&ts, TIME_UTC);
		return 1000000000U * ts.tv_sec + ts.tv_nsec;
	}
#elif defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__) // MSC or GCC on Intel
#	if _MSC_VER >= 1800
#		include <intrin.h>
#		pragma intrinsic(__rdtscp, __rdtsc, _mm_lfence)
#	elif defined(__GNUC__)
#		include <x86intrin.h>
#	else
#		error "Undefined compiler"
#	endif
 	VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) noexcept
	{	uint32_t _;
		// The RDTSCP instruction is not a serializing instruction, but it does wait until all previous instructions have executed.
		const uint64_t result = __rdtscp(&_);
		// �If software requires RDTSCP to be executed prior to execution of any subsequent instruction
		// (including any memory accesses), it can execute LFENCE immediately after RDTSCP� -
		// Intel� 64 and IA-32 Architectures Software Developer�s Manual: Vol.2B. P.4-553.
		_mm_lfence();
		return result;
	}
#elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
	VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) noexcept
	{	uint64_t result;
		asm volatile
		(	// too slow: "dmb ish\n\t" // Ensure all previous memory accesses are complete before reading the timer
			"isb\n\t" // Ensure the instruction stream is synchronized
			"mrs %0, cntvct_el0\n\t" // Read the current value of the system timer
			"isb\n\t" // Ensure the instruction stream is synchronized again
			: "=r"(result) // Output operand: result will hold the current timer value
			: // No input operands
			: "memory" // Clobber memory to ensure the compiler does not reorder instructions
		);
		return result;
	}
#elif __ARM_ARCH >= 6 // ARMv6 (RaspberryPi1B+)
#	include <cassert>
#	include <cerrno>
#	include <time.h> // for clock_gettime
#	include <fcntl.h>
#	include <sys/mman.h>
#	include <unistd.h>

	static auto get_peripheral_base()
	{	uint32_t result = 0;

		if (auto fp = open("/proc/device-tree/soc/ranges", O_RDONLY); fp >= 0)
		{	uint8_t buf[32];
			if (auto sz = read(fp, buf, sizeof(buf)); sz >= 32) // Raspberry Pi 4
			{	result = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | buf[11];
				assert(result == 0xFE00'0000);
			}
			else if (sz >= 12) // Raspberry Pi 1 - 3
			{	result = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
				assert(result == 0x2000'0000 || result == 0x3F00'0000);
			}
			else
			{	// SystemTimer_by_DevMem initial filed: Unknown format file '/proc/device-tree/soc/ranges'
				assert(false);
			}

			close(fp);
		}
		else
		{	// SystemTimer_by_DevMem initial filed.
			assert(false);
		}

		return result;
	}

	static const volatile uint32_t* get_timer_base()
	{	const volatile uint32_t *result = nullptr;

		if (const off_t peripheral_base = get_peripheral_base())
		{	constexpr auto TIMER_OFFSET = 0x3000;
			const auto timer_base = peripheral_base + TIMER_OFFSET;

			if (auto mem_fd = open("/dev/mem", O_RDONLY); mem_fd >= 0)
			{	const auto PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
				assert(PAGE_SIZE == 0x1000 && 0 == timer_base % PAGE_SIZE);

				if (void *mapped_base = mmap(nullptr, PAGE_SIZE, PROT_READ, MAP_SHARED, mem_fd, timer_base); mapped_base != MAP_FAILED)
				{	result = reinterpret_cast<volatile uint32_t *>(mapped_base);
				}
				else
				{	// SystemTimer_by_DevMem initial filed.
					assert(false);
				}
				close(mem_fd);
			}
		}

		if(!result)
		{	static constexpr char msg[] =
				"\x1B""[31m"
				"Attention! To ensure more accurate time measurements on this machine, the vi_timing library may require elevated privileges.\n"
				"Please try running the program with sudo.\n"
				"\x1B""[0m"
				"Error";
			perror(msg);
			errno = 0;
		}

		return result;
	}

	VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) noexcept
	{	VI_TM_TICK result = 0;

		static const volatile uint32_t *const timer_base = get_timer_base();
		if (timer_base)
		{	const uint64_t lo = timer_base[1]; // Timer low 32 bits
			const uint64_t hi = timer_base[2]; // Timer high 32 bits
			result = (hi << 32) | lo;
		}
		else
		{	struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
			result = uint64_t(1'000'000'000U) * ts.tv_sec + ts.tv_nsec;
		}

		return result;
	}
#elif defined(_WIN32) // Windows
#	include <Windows.h>
	VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) noexcept
	{	LARGE_INTEGER cnt;
		QueryPerformanceCounter(&cnt);
		return cnt.QuadPart;
	}
#elif defined(__linux__)
#	include <time.h>
	VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) noexcept
	{	struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		return 1'000'000'000U * ts.tv_sec + ts.tv_nsec;
	}
#else
#	error "You need to define function(s) for your OS and CPU"
#endif

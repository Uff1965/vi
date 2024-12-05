// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/********************************************************************\
'vi_timing' is a compact library designed for measuring the execution time of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was developed for experimental and educational purposes.
Please temper your expectations accordingly. If you encounter any bugs or have
suggestions for improvements, kindly contact me at programmer.amateur@proton.me.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. 
If not, see <https://www.gnu.org/licenses/gpl-3.0.html#license-text>.
\********************************************************************/

#include <vi_timing.h>

namespace
{
#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__) // MSC or GCC on Intel
#	if _MSC_VER >= 1800
#		include <intrin.h>
#		pragma intrinsic(__rdtscp, _mm_lfence)
#	elif defined(__GNUC__)
#		include <x86intrin.h>
#	else
#		error "Undefined compiler"
#	endif
	inline vi_tmTicks_t vi_tmGetTicks(void) noexcept
	{	std::uint32_t _; // Will be removed by the optimizer.
		const std::uint64_t result = __rdtscp(&_);
		//	«If software requires RDTSCP to be executed prior to execution of any subsequent instruction 
		//	(including any memory accesses), it can execute LFENCE immediately after RDTSCP» - 
		//	(Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes:
		//	1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4. Vol. 2B. P.4-553)
		_mm_lfence();
		return result;
	}
#elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
	inline vi_tmTicks_t vi_tmGetTicks(void) noexcept
	{	std::uint64_t result;
		__asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(result));
		return result;
	}
#elif __ARM_ARCH >= 6 // ARMv6 (RaspberryPi1B+)
#	include <time.h> // for clock_gettime
#	include <fcntl.h>
#	include <sys/mman.h>
#	include <unistd.h>

	volatile std::uint32_t *timer_base = []
		{	volatile std::uint32_t *result = nullptr;
			if (int mem_fd = open("/dev/mem", O_RDWR | O_SYNC); mem_fd < 0)
			{	assert(false); // Enhanced privileges are required (sudo).
			}
			else
			{	// Timer addresses in Raspberry Pi peripheral area
				constexpr off_t TIMER_BASE = 0x20003000;
				constexpr std::size_t BLOCK_SIZE = 4096;
				if (void *mapped_base = mmap(nullptr, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, TIMER_BASE); mapped_base == MAP_FAILED)
				{	assert(false);
				}
				else
				{	result = (volatile uint32_t *)mapped_base;
				}
				close(mem_fd);
			}

			return result;
		}();

	inline vi_tmTicks_t vi_tmGetTicks(void) noexcept
	{	vi_tmTicks_t result = 0;
		if (timer_base)
		{	const std::uint32_t lo = timer_base[1]; // Timer low 32 bits
			const std::uint32_t hi = timer_base[2]; // Timer high 32 bits
			result = ((std::uint64_t)hi << 32) | lo;
		}
		else
		{	struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
			result = 1'000'000'000U * ts.tv_sec + ts.tv_nsec;
		}

		return result;
	}
#elif defined(_WIN32) // Windows
#	include <Windows.h>
	inline vi_tmTicks_t vi_tmGetTicks(void) noexcept
	{	LARGE_INTEGER cnt;
		QueryPerformanceCounter(&cnt);
		return cnt.QuadPart;
	}
#elif defined(__linux__)
#	include <time.h>
	inline vi_tmTicks_t vi_tmGetTicks(void) noexcept
	{	struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		return 1'000'000'000U * ts.tv_sec + ts.tv_nsec;
	}
#else
#	error "You need to define function(s) for your OS and CPU"
#endif
}

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
vi_tmTicks_t VI_TM_CALL vi_tmClock() noexcept
{	return vi_tmGetTicks();
}
//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
#	include <cerrno>
#	include <time.h> // for clock_gettime
#	include <fcntl.h>
#	include <sys/mman.h>
#	include <unistd.h>

	auto get_peripheral_base()
	{	std::uint32_t result = 0;

		if (auto fp = open("/proc/device-tree/soc/ranges", O_RDONLY); fp >= 0)
		{	std::uint8_t buf[32];
			if (auto sz = read(fp, buf, sizeof(buf)); sz >= 32) // Raspberry Pi 4
			{	result = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | buf[11];
				assert(result == 0xFE000000);
			}
			else if (sz >= 12) // Raspberry Pi 1 - 3
			{	result = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
				assert(result == 0x20000000 || result == 0x3F000000);
			}
			else
			{	//printf("SystemTimer_by_DevMem initial filed: Unknown format file \'/proc/device-tree/soc/ranges\'\n");
			}

			close(fp);
		}
		else
		{	//perror("SystemTimer_by_DevMem initial filed");
		}

		return result;
	}

	volatile std::uint32_t* get_timer_base()
	{	volatile std::uint32_t *result = nullptr;

		errno = 0;

		if (const off_t peripheral_base = get_peripheral_base())
		{	constexpr auto TIMER_OFFSET = 0x3000;
			const auto timer_base = peripheral_base + TIMER_OFFSET;

			if (auto mem_fd = open("/dev/mem", O_RDONLY); mem_fd >= 0)
			{	const auto PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
				assert(PAGE_SIZE == 0x1000 && 0 == timer_base % PAGE_SIZE);

				if (void *mapped_base = mmap(nullptr, PAGE_SIZE, PROT_READ, MAP_SHARED, mem_fd, timer_base); mapped_base != MAP_FAILED)
				{	result = reinterpret_cast<volatile std::uint32_t *>(mapped_base);
				}
				else
				{	//perror("SystemTimer_by_DevMem initial filed");
				}
				close(mem_fd);
			}
		}

		if(!result)
		{	//perror("SystemTimer_by_DevMem initial filed"); // Enhanced privileges are required (sudo).
			static constexpr char msg[] =
				"Attention! To ensure more accurate time measurements on this machine, "
				"the vi_timing library may require elevated privileges. Please try running the program with sudo.";
			perror(msg);
		}

		return result;
	}

	inline vi_tmTicks_t vi_tmGetTicks(void) noexcept
	{	vi_tmTicks_t result = 0;

		static volatile std::uint32_t *const timer_base = get_timer_base();
		if (timer_base)
		{	const std::uint64_t lo = timer_base[1]; // Timer low 32 bits
			const std::uint64_t hi = timer_base[2]; // Timer high 32 bits
			result = (hi << 32) | lo;
		}
		else
		{	struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
			result = std::uint64_t(1'000'000'000U) * ts.tv_sec + ts.tv_nsec;
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

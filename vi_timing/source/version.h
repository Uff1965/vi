#ifndef VI_TIMING_SOURCE_VERSION_H
#	define VI_TIMING_SOURCE_VERSION_H
#	pragma once

#include <cstdint>

#define VI_TM_VERSION "0.10.0"
#define VI_TM_VERSION_MAJOR 0
#define VI_TM_VERSION_MINOR 10
#define VI_TM_VERSION_PATCH 0

#define GIT_DESCRIBE "v0.10.0-3-g96b37d4-dirty"
#define GIT_COMMIT "96b37d49d235140e86f6f6c246bc7f166ab773aa"
#define GIT_DATETIME "2025-07-26 13:56:02 +0300"
#define GIT_IS_DIRTY 1

namespace misc
{
	// Build Number Generator: Formats the build number using the compilation date and time. Example: 2506170933U.
 
	// Updates the global variable based on the last compilation time.
	unsigned build_number_updater(const char(&date)[12], const char(&time)[9]);
	// Next line will be called in each translation unit.
	static const std::uint32_t dummy_build_number_updater = build_number_updater(__DATE__, __TIME__);

	// Returns a number based on the date and time of the last compilation.
	std::uint32_t build_number_get();
}

#endif // #ifndef VI_TIMING_SOURCE_VERSION_H

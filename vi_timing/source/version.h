#ifndef VI_TIMING_SOURCE_VERSION_H
#	define VI_TIMING_SOURCE_VERSION_H
#	pragma once

#include <cstdint>

#define PROJECT_VERSION "0.10.0"
#define PROJECT_VERSION_MAJOR 0
#define PROJECT_VERSION_MINOR 10
#define PROJECT_VERSION_PATCH 0

#define GIT_DESCRIBE "v0.10.0-2-g7e8f05c-dirty"
#define GIT_COMMIT "7e8f05c7882d9405f4b5fe3b4a80ab6606ac7326"
#define GIT_DATETIME "2025-07-24 21:03:53 +0300"
#define GIT_IS_DIRTY 1
#define GIT_VERSION_NUMBER "0.10.0"

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

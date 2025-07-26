#ifndef VI_TIMING_SOURCE_VERSION_H
#	define VI_TIMING_SOURCE_VERSION_H
#	pragma once

#include <cstdint>

inline constexpr unsigned int VI_TM_VERSION_MAJOR = 0;
inline constexpr unsigned int VI_TM_VERSION_MINOR = 10;
inline constexpr unsigned int VI_TM_VERSION_PATCH = 0;

inline constexpr char GIT_DESCRIBE[] = "v0.10.0-5-g980964a";
inline constexpr char GIT_COMMIT[] = "980964afa81331a2eb1aeed637ecf97af49ef047";
inline constexpr char GIT_DATETIME[] = "2025-07-26 22:15:41 +0300";
inline constexpr int GIT_IS_DIRTY = 0;

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

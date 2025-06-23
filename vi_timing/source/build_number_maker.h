#ifndef VI_TIMING_SOURCE_BUILD_NUMBER_MAKER_H
#	define VI_TIMING_SOURCE_BUILD_NUMBER_MAKER_H
#	pragma once

namespace misc
{
	// Updates the global variable based on the last compilation time.
	unsigned build_number_updater(const char(&date)[12], const char(&time)[9]);
	// Next line will be called in each translation unit.
	static const unsigned dummy_build_number_updater = build_number_updater(__DATE__, __TIME__);

	// Returns a number based on the date and time of the last compilation.
	unsigned build_number_get();
}

#endif // #ifndef VI_TIMING_SOURCE_BUILD_NUMBER_MAKER_H

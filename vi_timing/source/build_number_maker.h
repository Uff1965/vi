#ifndef VI_TIMING_SOURCE_BUILD_NUMBER_H
#	define VI_TIMING_SOURCE_BUILD_NUMBER_H
#	pragma once

namespace misc
{
	unsigned build_number_updater(const char(&date)[12], const char(&time)[9]); // Converts the date and time to a monotonically increasing number. Stores it in BUILD_NUMBER.
	unsigned build_number_get();
	static const unsigned dummy_build_number_updater = build_number_updater(__DATE__, __TIME__);
}

#endif // #ifndef VI_TIMING_SOURCE_BUILD_NUMBER_H

// vi_timing_proxy.h - Proxy header for the vi_timing library

#ifndef _VI_TIMING_PROXY_H_
#	define _VI_TIMING_PROXY_H_ "3.0.0.1"
#	pragma once

// Check if the vi/timing.h header is available and include it if possible
#	if !defined(VI_TM_DISABLE) && __has_include(<vi_timing/vi_timing.h>)
#		include <vi_timing/vi_timing.h>
#	else
		// Auxiliary macros for combining tokens and generating a unique identifier.
#		define VI_STR_GUM_AUX( a, b ) a##b
#		define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#		define VI_UNIC_ID( prefix ) VI_STR_GUM( prefix, __LINE__ )

		// Fallback macros for timing functions
#		define VI_TM_INIT(...) static const int VI_UNIC_ID(_vi_tm_) = 0
#		define VI_TM(...) static const int VI_UNIC_ID(_vi_tm_) = 0
#		define VI_TM_FUNC ((void)0)
#		define VI_TM_REPORT(...) ((void)0)
#		define VI_TM_RESET ((void)0)
#		define VI_TM_FULLVERSION ""
#	endif

#endif // #ifndef _VI_TIMING_PROXY_H_

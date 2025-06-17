// vi_timing_proxy.h - Proxy header for the vi_timing library

#ifndef _VI_TIMING_PROXY_H_
#	define _VI_TIMING_PROXY_H_
#	pragma once

// Check if the vi/timing.h header is available and include it if possible
#	if !defined(VI_TM_DISABLE) && __has_include(<vi_timing/vi_timing.h>)
#		include <vi_timing/vi_timing.h>
#	else
		// Auxiliary macros for generating a unique identifier.
#		ifndef __COUNTER__ // __COUNTER__ is not included in the standard yet.
#			define __COUNTER__ __LINE__
#		endif
#		define VI_STR_GUM_AUX( a, b ) a##b
#		define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#		define VI_UNIC_ID( prefix ) VI_STR_GUM( prefix, __COUNTER__ )

		// Fallback macros for timing functions
#		define VI_TM_INIT(...) static const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM(...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_FUNC ((void)0)
#		define VI_TM_REPORT(...) ((void)0)
#		define VI_TM_RESET ((void)0)
#		define VI_TM_FULLVERSION ""
#	endif

#endif // #ifndef _VI_TIMING_PROXY_H_

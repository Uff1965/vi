#ifndef VI_TIMING_PROXY_H_
#	define VI_TIMING_PROXY_H_ "3.0.0.1"
#	pragma once

#	if !defined(VI_TM_DISABLE) && __has_include(<vi_timing/timing.h>)
#		include <vi_timing/timing.h>
#	else
#		define VI_TM_DISABLE 1

#		define VI_STR_AUX( a, b ) a##b
#		define VI_STR( a, b ) VI_STR_AUX( a, b )
#		define VI_MAKE_UNIC_ID( prefix ) VI_STR( prefix, __LINE__ )

#		define VI_TM_INIT(...) int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = 0
#		define VI_TM(...) int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = 0
#		define VI_TM_REPORT(...) ((void)0)
#		define VI_TM_FUNC ((void)0)
#		define VI_TM_CLEAR ((void)0)
#	endif

#endif // #ifndef VI_TIMING_PROXY_H_

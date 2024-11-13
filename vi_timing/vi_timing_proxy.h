#ifndef VI_TIMING_PROXY_H_
#	define VI_TIMING_PROXY_H_ "3.0.0.1"
#	pragma once

#	if __has_include(<vi_timing/timing.h>)
#		include <vi_timing/timing.h>
#	else
#		ifndef VI_TM_DISABLE
#			define VI_TM_DISABLE 1
#		endif

#		define VI_STR_GUM_AUX( a, b ) a##b
#		define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#		define VI_MAKE_UNIC_ID( prefix ) VI_STR_GUM( prefix, __LINE__ )

#		define VI_TM_INIT(...) int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = 0
#		define VI_TM(...) int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = 0
#		define VI_TM_REPORT(...) ((void)0)
#		define VI_TM_FUNC ((void)0)
#		define VI_TM_CLEAR ((void)0)
#		define VI_TM_INFO(f) NULL
#	endif

#endif // #ifndef VI_TIMING_PROXY_H_

#ifndef VI_TIMING_PROXY_H_
#	define VI_TIMING_PROXY_H_ "0.1.1.1"
#	pragma once

#	if __has_include("vi_timing.h")
#		include "vi_timing.h"
#	elif __has_include(<vi_timing/vi_timing.h>)
#		include <vi_timing/vi_timing.h>
#	else
#		ifndef VI_TM_DISABLE
#			define VI_TM_DISABLE "Header file not found."
#		endif

#		define VI_STR_GUM_AUX( a, b ) a##b
#		define VI_STR_GUM( a, b ) VI_STR_GUM_AUX( a, b )
#		define VI_MAKE_ID( prefix ) VI_STR_GUM( prefix, __LINE__ )

#		define VI_TM_INIT(...) static const int VI_MAKE_ID(_vi_tm_) = 0
#		define VI_TM(...) static const int VI_MAKE_ID(_vi_tm_) = 0
#		define VI_TM_REPORT(...) ((void)0)
#		define VI_TM_FUNC ((void)0)
#		define VI_TM_CLEAR ((void)0)
#		define VI_TM_INFO(f) NULL
#	endif
#endif // #ifndef VI_TIMING_PROXY_H_

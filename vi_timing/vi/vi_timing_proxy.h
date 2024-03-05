#ifndef _CM_TIMING_PROXY_H_
#	define _CM_TIMING_PROXY_H_ "1.0.0.1"
#	pragma once

#	if defined __cplusplus && !defined(CM_TM_DISABLE) && __has_include(<cm_timing/cm_timing.h>)
#		include <cm_timing/cm_timing.h>
#	else
#		define CM_TM_DISABLE 1
#		define CM_TM(...) ((void)0)
#		define CM_TM_FUNC ((void)0)
#		define CM_TM_INIT(s, ...)
#		define CM_TM_REPORT(...) ((void)0)
#		define CM_TM_CLEAR ((void)0)
#	endif

#endif // #ifndef _CM_TIMING_PROXY_H_

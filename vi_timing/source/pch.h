#ifndef VI_VI_TIMING_PCH_H
#	define VI_VI_TIMING_PCH_H
#	pragma once

#	include "../vi_timing_c.h"
#	include "build_number_maker.h"
#	include "misc.h"

#	include <array>
#	include <cassert>
#	include <cstdint>
#	include <stdio.h>
#	include <thread>

#define verify(v) [](bool b) noexcept { assert(b); return b; }(v) // Define for displaying the __FILE__ and __LINE__ during debugging.

#endif // #ifndef VI_VI_TIMING_PCH_H

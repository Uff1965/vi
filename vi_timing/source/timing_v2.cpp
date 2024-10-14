// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/********************************************************************\
'vi_timing' is a small library for measuring the time execution of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was created to experiment for educational purposes.
Do not expect much from it. If you spot a bug or can suggest any
improvement to the code, please email me at eMail:programmer.amateur@proton.me.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. 
If not, see <https://www.gnu.org/licenses/gpl-3.0.html#license-text>.
\********************************************************************/

#include <timing_v2.h>

#include <algorithm>
#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
	struct item_t
	{
		vi_tmAtomicTicks_t ticks_{};
		std::size_t amount_{};
		std::size_t calls_cnt_{};
		void clear()
		{
			amount_ = 0;
			calls_cnt_ = 0;
			ticks_ = 0;
		}
	};
}

struct vi_tmTimer_t
{
	vi_tmAtomicTicks_t s_dummy_ticks{ 0 };
	std::mutex s_instance_guard;
	std::unordered_map<std::string, item_t> s_instance;
};

vi_tmTimer_t* VI_CALL vi_tmCreate_v2(std::size_t n)
{
	std::unique_ptr<vi_tmTimer_t> result{ new vi_tmTimer_t };
	result->s_instance.reserve(n);
	return result.release();
}

vi_tmTimer_t *VI_CALL vi_tmGlobal_v2(void)
{
	static std::unique_ptr<vi_tmTimer_t> global{ vi_tmCreate_v2(64) };
	return global.get();
}

vi_tmAtomicTicks_t* VI_CALL vi_tmItem_v2(vi_tmTimer_t* tm, const char* name, std::size_t amount)
{	assert(tm);
	vi_tmAtomicTicks_t* result;
	if (nullptr != name)
	{
		std::scoped_lock lg_{ tm->s_instance_guard };
		auto& item = tm->s_instance[name];
		item.calls_cnt_++;
		item.amount_ += amount;
		result = &item.ticks_;
	}
	else
	{
		result = &tm->s_dummy_ticks;
	}
	return result;
}

int VI_CALL vi_tmResults_v2(vi_tmTimer_t* tm, vi_tmLogRAW_t fn, void* data)
{	assert(tm);
	auto result = -1;
	std::scoped_lock lg_{ tm->s_instance_guard };
	for (const auto& [name, item] : tm->s_instance)
	{	assert(item.calls_cnt_ && item.amount_ >= item.calls_cnt_);
		if (!name.empty() && 0 == fn(name.c_str(), item.ticks_, item.amount_, item.calls_cnt_, data))
		{	result = 0;
			break;
		}
	}
	return result;
}

void VI_CALL vi_tmClear_v2(vi_tmTimer_t* tm)
{	assert(tm);
	std::scoped_lock lg{ tm->s_instance_guard };
	std::for_each(begin(tm->s_instance), end(tm->s_instance), [](auto &i) { i.second.clear(); });
	tm->s_dummy_ticks = 0U;
}

void VI_CALL vi_tmClose_v2(vi_tmTimer_t *tm)
{	assert(tm);
	delete tm;
}

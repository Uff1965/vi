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

#include <timing.h>

#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map>

struct item_t
{
	std::atomic<vi_tmTicks_t> ticks_{ 0 };
	std::size_t amount_{ 0 };
	std::size_t calls_cnt_{ 0 };
};

decltype(item_t::ticks_) s_dummy_ticks{ 0 };
std::mutex s_instance_guard;
std::unordered_map<std::string, item_t> s_instance;

std::atomic<vi_tmTicks_t>* vi_tmItem(const char* name, std::size_t amount)
{
	std::atomic<vi_tmTicks_t>* result;
	if (nullptr != name)
	{
		std::scoped_lock lg_{ s_instance_guard };
		auto& item = s_instance[name];
		item.calls_cnt_++;
		item.amount_ += amount;
		result = &item.ticks_;
	}
	else
	{
		result = &s_dummy_ticks;
	}
	return result;
}

int vi_tmResults(vi_tmLogRAW_t fn, void* data)
{
	auto result = -1;
	std::scoped_lock lg_{ s_instance_guard };
	for (const auto& [name, item] : s_instance)
	{
		assert(item.calls_cnt_ && item.amount_ >= item.calls_cnt_);
		if (!name.empty() && 0 == fn(name.c_str(), item.ticks_, item.amount_, item.calls_cnt_, data))
		{
			result = 0;
			break;
		}
	}
	return result;
}

void vi_tmInit(std::size_t n)
{
	std::scoped_lock lg_{ s_instance_guard };
	assert(s_instance.empty());
	s_instance.reserve(n);
}

void vi_tmClear(void)
{
	std::scoped_lock lg_{ s_instance_guard };
	for (auto& [name, item] : s_instance)
	{
		item.amount_ = 0;
		item.calls_cnt_ = 0;
		item.ticks_ = 0;
	}

	s_dummy_ticks = 0U;
}

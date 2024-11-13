// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/********************************************************************\
'vi_timing' is a compact library designed for measuring the execution time of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was developed for experimental and educational purposes.
Please temper your expectations accordingly. If you encounter any bugs or have
suggestions for improvements, kindly contact me at programmer.amateur@proton.me.

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

//-V:assert:2570

#include <timing.h>

#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map> // Unordered associative containers: "Rehashing invalidates iterators, <...> but does not invalidate pointers or references to elements".

namespace
{
	struct item_t
	{	vi_tmAtomicTicks_t amount_{ 0 };
		std::size_t counter_{ 0 };
		std::size_t calls_cnt_{ 0 };
		void clear() noexcept
		{	amount_ = counter_ = calls_cnt_ = 0;
		};
	};

	auto lock_and_get_pointer()
	{	static std::mutex storage_guard;
		static auto storage = []
		{	std::unordered_map<std::string, item_t> result;
			result.max_load_factor(0.7F);
			result.reserve(64);
			return result;
		}();

		class pointer_to_locked_item_t
		{	std::scoped_lock<std::mutex> lock_{ storage_guard };
		public:
			pointer_to_locked_item_t() = default;
			pointer_to_locked_item_t(const pointer_to_locked_item_t &) = delete;
			pointer_to_locked_item_t &operator=(const pointer_to_locked_item_t &) = delete;
			auto &operator[](const char *key) const & { return storage[key]; }
			auto &operator*() const & { return storage; }
			auto &operator*() const && = delete;
			auto operator->() const & { return &storage; }
			auto operator->() const && = delete;
		};

		return pointer_to_locked_item_t{};
	};

	vi_tmAtomicTicks_t s_dummy_amount{};

} // namespace

void VI_TM_CALL vi_tmInit(std::size_t n)
{	auto locked_ptr = lock_and_get_pointer();
	assert(locked_ptr->empty());
	locked_ptr->reserve(n);
}

void VI_TM_CALL vi_tmFinit()
{	auto locked_ptr = lock_and_get_pointer();
	locked_ptr->clear();
	s_dummy_amount = 0U;
}

vi_tmAtomicTicks_t* VI_TM_CALL vi_tmItem(const char* name, std::size_t amount)
{	vi_tmAtomicTicks_t* result;
	if (nullptr != name)
	{	auto locked_ptr = lock_and_get_pointer();
		auto &item = locked_ptr[name];
		item.calls_cnt_ += 1;
		item.counter_ += amount;
		result = &item.amount_;
	}
	else
	{	result = &s_dummy_amount;
	}
	return result;
}

int VI_TM_CALL vi_tmResults(vi_tmLogRAW_t fn, void* data)
{	int result = -1;
	auto locked_ptr = lock_and_get_pointer();
	for (const auto& [name, item] : *locked_ptr)
	{	assert(item.counter_ >= item.calls_cnt_);
		assert((0 == item.amount_) == (0 == item.calls_cnt_));
		if (!name.empty() && 0 == fn(name.c_str(), item.amount_, item.counter_, item.calls_cnt_, data))
		{	result = 0;
			break;
		}
	}
	return result;
}

void VI_TM_CALL vi_tmClear(const char* name)
{	if (auto locked_ptr = lock_and_get_pointer(); !name)
	{	for (auto &[_, item] : *locked_ptr)
		{	item.clear();
		}
		s_dummy_amount = 0U;
	}
	else if (const auto [it, b] = locked_ptr->try_emplace(name); !b)
	{	it->second.clear();
	}
}

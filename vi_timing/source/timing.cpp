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

#include <timing.h>

#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map> // Unordered associative containers: "Rehashing invalidates iterators, <...> but does not invalidate pointers or references to elements".

namespace
{
	struct item_t
	{	vi_tmAtomicTicks_t total_ = 0U;
		std::size_t counter_ = 0U;
		std::size_t calls_cnt_ = 0U;
		void clear() noexcept
		{	total_ = counter_ = calls_cnt_ = 0U;
		};
	};

	constexpr auto MAX_LOAD_FACTOR = 0.7F;
	constexpr auto DEF_CAPACITY = 64U;
	using storage_t = std::unordered_map<std::string, item_t>;

} // namespace

struct vi_tmInstance_t
{	std::mutex storage_guard_;
	storage_t storage_;
	vi_tmAtomicTicks_t total_dummy_ = 0U;

	explicit vi_tmInstance_t(std::size_t reserve)
	{	storage_.max_load_factor(MAX_LOAD_FACTOR);
		storage_.reserve(reserve);
	}

	static vi_tmInstance_t& global()
	{	static vi_tmInstance_t inst{ DEF_CAPACITY };
		return inst;
	}

	void init(size_t reserve)
	{	std::scoped_lock lock{ storage_guard_ };
		storage_.reserve(reserve);
	}

	vi_tmAtomicTicks_t& total(const char *name, std::size_t cnt)
	{	if (name)
		{	std::scoped_lock lock{ storage_guard_ };
			auto &item = storage_[name];
			item.calls_cnt_ += 1;
			item.counter_ += cnt;
			return item.total_;
		}

		return total_dummy_;
	}

	int results(vi_tmLogRAW_t fn, void *data)
	{	std::scoped_lock lock{ storage_guard_ };
		for (const auto &[name, item] : storage_)
		{	assert(item.counter_ >= item.calls_cnt_ && ((0 == item.total_) == (0 == item.calls_cnt_)));
			if (!name.empty() && 0 == fn(name.c_str(), item.total_, item.counter_, item.calls_cnt_, data))
			{	return 0;
			}
		}
		return -1;
	}
	
	void clear(const char *name)
	{	std::scoped_lock lock{ storage_guard_ };
		if (!name)
		{	total_dummy_ = 0U;
			for (auto &[_, item] : storage_)
			{	item.clear();
			}
		}
		else if (const auto [it, b] = storage_.try_emplace(name); !b)
		{	it->second.clear();
		}
	}

	friend vi_tmInstance_t& from_handle(vi_tmInstance_t *p)
	{	return p? *p: global();
	}
}; // struct vi_tmInstance_t

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

void VI_TM_CALL vi_tmInit(std::size_t reserve)
{	vi_tmInstance_t::global().init(reserve);
}

void VI_TM_CALL vi_tmFinit()
{	vi_tmInstance_t::global().clear(nullptr);
}

VI_TM_HANDLE VI_TM_CALL vi_tmCreate(std::size_t reserve)
{	return new vi_tmInstance_t{ reserve };
}

void VI_TM_CALL vi_tmClose(VI_TM_HANDLE h)
{	delete h;
}

vi_tmAtomicTicks_t* VI_TM_CALL vi_tmTotalTicks(VI_TM_HANDLE h, const char* name, std::size_t amount) noexcept
{	return &from_handle(h).total(name, amount);
}

void VI_TM_CALL vi_tmClear(VI_TM_HANDLE h, const char* name) noexcept
{	from_handle(h).clear(name);
}

int VI_TM_CALL vi_tmResults(VI_TM_HANDLE h, vi_tmLogRAW_t fn, void *data)
{	return from_handle(h).results(fn, data);
}

//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

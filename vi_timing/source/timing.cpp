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

#include "../vi_timing.h"
#include "build_number_maker.h"

#include <atomic>
#include <cassert>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map> // unordered_map: "does not invalidate pointers or references to elements".

#ifdef __STDC_NO_ATOMICS__
//	At the moment Atomics are available in Visual Studio 2022 with the /experimental:c11atomics flag.
//	"we left out support for some C11 optional features such as atomics" [Microsoft
//	https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#	error "Atomic objects and the atomic operation library are not supported."
#endif

namespace
{
	struct measuring_t
	{	std::atomic<VI_TM_TICKDIFF> total_ = 0U;
		std::atomic<std::size_t> counter_ = 0U;
		std::atomic<std::size_t> calls_cnt_ = 0U;
		void add(VI_TM_TICKDIFF tick_diff, std::size_t amount) noexcept
		{	total_.fetch_add(tick_diff, std::memory_order_relaxed);
			counter_.fetch_add(amount, std::memory_order_relaxed);
			calls_cnt_.fetch_add(1, std::memory_order_relaxed);
		}
		void get(VI_TM_TICKDIFF &total, VI_TM_CNT &amount, VI_TM_CNT &calls_cnt)
		{	total = total_.load();
			amount = counter_.load();
			calls_cnt = calls_cnt_.load();
		}
		void clear() noexcept
		{	total_ = 0U;
			counter_ = 0U;
			calls_cnt_ = 0U;
		}
	};

	constexpr auto MAX_LOAD_FACTOR = 0.7F;
	constexpr std::size_t DEFAULT_STORAGE_CAPACITY = 64U;
	using storage_t = std::unordered_map<std::string, measuring_t>;
}

struct vi_tmMeasuring_t: storage_t::value_type {/**/};

struct vi_tmJournal_t
{	std::mutex storage_guard_;
	storage_t storage_;

	explicit vi_tmJournal_t()
	{	storage_.max_load_factor(MAX_LOAD_FACTOR);
		storage_.reserve(DEFAULT_STORAGE_CAPACITY);
	}

	static vi_tmJournal_t& global()
	{	static vi_tmJournal_t inst;
		return inst;
	}

	int init()
	{	return 0;
	}

	storage_t::value_type& get_item(const char *name)
	{	std::lock_guard lock{ storage_guard_ };
		return *storage_.try_emplace(name).first;
	}

	int result(const char *name, VI_TM_TICKDIFF &time, std::size_t &amount, std::size_t &calls_cnt)
	{	assert(name);
		if (nullptr == name)
		{	return -1;
		}

		{	std::lock_guard lock{ storage_guard_ };

			if (auto it = storage_.find(name); it != storage_.end())
			{	assert(it->first == name);
				time = it->second.total_.load(std::memory_order_relaxed);
				amount = it->second.counter_.load(std::memory_order_relaxed);
				calls_cnt = it->second.calls_cnt_.load(std::memory_order_relaxed);

				return 0;
			}
		}

		time = 0U;
		amount = 0U;
		calls_cnt = 0U;

		return 1;
	}

	int enumerate(vi_tmMeasuringEnumCallback_t fn, void *data)
	{	std::lock_guard lock{ storage_guard_ };

		for (auto &it : storage_)
		{	assert(it.second.counter_ >= it.second.calls_cnt_ && ((0 == it.second.total_) == (0 == it.second.calls_cnt_)));
			if (!it.first.empty())
			{	if (const auto interrupt = std::invoke(fn, static_cast<VI_TM_HMEASURING>(&it), data); 0 != interrupt)
				{	return interrupt;
				}
			}
		}

		return 0;
	}
	
	void clear(const char *name = nullptr)
	{	std::lock_guard lock{ storage_guard_ };

		if (!name)
		{	for (auto &[_, item] : storage_)
			{	item.clear();
			}
		}
		else if (const auto [it, b] = storage_.try_emplace(name); !b)
		{	it->second.clear();
		}
	}

	friend vi_tmJournal_t& from_handle(VI_TM_HJOURNAL h)
	{	return h? *h: global();
	}
}; // struct vi_tmJournal_t

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

int VI_TM_CALL vi_tmInit()
{	return vi_tmJournal_t::global().init();
}

void VI_TM_CALL vi_tmFinit(void)
{	vi_tmJournal_t::global().clear();
}

VI_TM_HJOURNAL VI_TM_CALL vi_tmJournalCreate()
{	try
	{	return new vi_tmJournal_t{};
	}
	catch (const std::bad_alloc &)
	{	return nullptr;
	}
}

void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOURNAL h)
{	delete h;
}

VI_TM_HMEASURING VI_TM_CALL vi_tmMeasuring(VI_TM_HJOURNAL h, const char *name)
{	auto& itm = from_handle(h).get_item(name);
	return static_cast<VI_TM_HMEASURING>(&itm);
}

void VI_TM_CALL vi_tmMeasuringAdd(VI_TM_HMEASURING measuring, VI_TM_TICKDIFF tick_diff, std::size_t amount) noexcept
{	assert(measuring);
	if (measuring)
	{	measuring->second.add(tick_diff, amount);
	}
}

void VI_TM_CALL vi_tmMeasuringGet(VI_TM_HMEASURING measuring, const char **name, VI_TM_TICKDIFF *total, VI_TM_CNT *amount, VI_TM_CNT *calls_cnt)
{	assert(measuring);
	if (measuring)
	{	if (name)
		{	*name = measuring->first.c_str();
		}
		VI_TM_CNT _ = 0U;
		measuring->second.get(total ? *total : _, amount ? *amount : _, calls_cnt ? *calls_cnt : _);
	}
}

void VI_TM_CALL vi_tmMeasuringClear(VI_TM_HMEASURING measuring)
{	assert(measuring);
	if (measuring)
	{	measuring->second.clear();
	}
}

void VI_TM_CALL vi_tmJournalClear(VI_TM_HJOURNAL journal, const char* name) noexcept
{	from_handle(journal).clear(name);
}

int VI_TM_CALL vi_tmMeasuringEnum(VI_TM_HJOURNAL h, vi_tmMeasuringEnumCallback_t fn, void *data)
{	return from_handle(h).enumerate(fn, data);
}

//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
